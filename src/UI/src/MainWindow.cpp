#include "UI/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QEasingCurve>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPdfWriter>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
#include <set>

#include "Core/Ports/ISyncClient.h"
#include "Core/Services/SyncService.h"
#include "Editor/EditorWidget.h"
#include "Spelling/SpellChecker.h"
#include "Sync/HttpSyncClient.h"
#include "UI/BreadcrumbWidget.h"
#include "UI/FindBar.h"
#include "UI/FolderTreeWidget.h"
#include "UI/HelpDialog.h"
#include "UI/InsertTagDialog.h"
#include "UI/NewNoteDialog.h"
#include "UI/PreviewWidget.h"
#include "UI/SearchDialog.h"
#include "UI/SettingsDialog.h"
#include "UI/ShortcutsDialog.h"
#include "UI/SidebarWidget.h"
#include "UI/SyncLoginDialog.h"
#include "UI/Theme.h"
#include "UI/WelcomeScreen.h"

namespace noctis::ui {

MainWindow::MainWindow(AppContext context, QWidget* parent)
    : QMainWindow(parent), context_(std::move(context)) {
    setWindowTitle("Noctis");

    autosaveTimer_ = new QTimer(this);
    autosaveTimer_->setSingleShot(true);
    autosaveTimer_->setInterval(500);
    connect(autosaveTimer_, &QTimer::timeout, this, &MainWindow::flushAutosave);

    setupLayout();
    setupMenus();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupLayout() {
    sidebar_ = new SidebarWidget(context_.repository, context_.favoritesService,
                                  context_.recentsService, context_.tagService, context_.settings,
                                  context_.rootFolder, this);
    editor_ = new editor::EditorWidget(this);
    editor_->setTabWidth(context_.settings.getInt("editor.tabWidth").value_or(4));

    // El diccionario (es_ES.aff/.dic) se copia junto al ejecutable durante el
    // build (ver UI/CMakeLists.txt); si no está — instalación incompleta, u
    // otro idioma no soportado todavía — SpellChecker degrada en silencio y
    // el editor queda igual, solo sin subrayado.
    QString dictionaryDir = qApp->applicationDirPath() + "/dictionaries/";
    spellChecker_ = std::make_unique<spelling::SpellChecker>(
        (dictionaryDir + "es_ES.aff").toStdString(), (dictionaryDir + "es_ES.dic").toStdString());
    editor_->setSpellChecker(spellChecker_.get());

    preview_ = new PreviewWidget(this);

    // Ctrl+rueda sobre cualquiera de los dos paneles: ambos hacen zoom juntos.
    connect(editor_, &editor::EditorWidget::zoomStepRequested, this,
            [this](int direction) { setZoomLevel(zoomLevel_ + direction); });
    connect(preview_, &PreviewWidget::zoomStepRequested, this,
            [this](int direction) { setZoomLevel(zoomLevel_ + direction); });
    setZoomLevel(context_.settings.getInt("ui.zoomLevel").value_or(0));

    connect(sidebar_, &SidebarWidget::noteActivated, this, &MainWindow::openNote);
    connect(sidebar_, &SidebarWidget::createNoteRequested, this, &MainWindow::createNote);
    connect(sidebar_, &SidebarWidget::createSubfolderRequested, this,
            &MainWindow::createSubfolder);
    connect(sidebar_, &SidebarWidget::renameFolderRequested, this, &MainWindow::renameFolder);
    connect(sidebar_, &SidebarWidget::deleteFolderRequested, this, &MainWindow::deleteFolder);
    connect(sidebar_, &SidebarWidget::renameNoteRequested, this, &MainWindow::renameNote);
    connect(sidebar_, &SidebarWidget::deleteNoteRequested, this, &MainWindow::deleteNote);
    connect(sidebar_, &SidebarWidget::tagActivated, this, &MainWindow::openTagSearch);
    connect(sidebar_, &SidebarWidget::settingsRequested, this, &MainWindow::openSettingsDialog);
    connect(sidebar_, &SidebarWidget::shortcutsRequested, this, &MainWindow::openShortcutsDialog);
    connect(sidebar_, &SidebarWidget::helpRequested, this, &MainWindow::openHelpDialog);

    connect(editor_, &editor::EditorWidget::contentChanged, this, &MainWindow::scheduleAutosave);
    connect(editor_, &editor::EditorWidget::contentChanged, this, &MainWindow::updatePreview);
    connect(editor_, &editor::EditorWidget::contentChanged, this, [this] {
        if (loadingNote_) return;
        sidebar_->tree()->setActiveNoteDirty(true);
        statusSaveLabel_->setText(tr("Editando…"));

        int index = tabBar_->currentIndex();
        if (index < 0) return;
        QString text = tabBar_->tabText(index);
        if (!text.endsWith(QString::fromUtf8(" ●"))) {
            tabBar_->setTabText(index, text + QString::fromUtf8(" ●"));
        }
    });

    contentSplitter_ = new QSplitter(this);
    contentSplitter_->addWidget(editor_);
    contentSplitter_->addWidget(preview_);
    contentSplitter_->setStretchFactor(0, 1);
    contentSplitter_->setStretchFactor(1, 1);

    contentOpacity_ = new QGraphicsOpacityEffect(contentSplitter_);
    contentOpacity_->setOpacity(1.0);
    contentSplitter_->setGraphicsEffect(contentOpacity_);

    // Sync Scroll: en modo Split, editor y preview se desplazan juntos. No
    // hay una correspondencia 1:1 de renglones entre texto plano y HTML
    // renderizado, así que se mapea por proporción (value/maximum) en vez de
    // por posición absoluta. El flag evita el eco infinito entre ambas
    // conexiones (mover uno dispara moverse el otro, que dispararía...).
    auto syncScroll = [this](QScrollBar* source, QScrollBar* target, int value) {
        if (syncingScroll_) return;
        if (source->maximum() <= 0) return;

        syncingScroll_ = true;
        double ratio = static_cast<double>(value) / source->maximum();
        target->setValue(qRound(ratio * target->maximum()));
        syncingScroll_ = false;
    };
    connect(editor_->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this, syncScroll](int value) {
                syncScroll(editor_->verticalScrollBar(), preview_->verticalScrollBar(), value);
            });
    connect(preview_->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this, syncScroll](int value) {
                syncScroll(preview_->verticalScrollBar(), editor_->verticalScrollBar(), value);
            });

    tabBar_ = new QTabBar(this);
    tabBar_->setObjectName("noteTabBar");
    tabBar_->setTabsClosable(true);
    tabBar_->setExpanding(false);
    tabBar_->setDrawBase(false);
    connect(tabBar_, &QTabBar::currentChanged, this, &MainWindow::handleTabChanged);
    connect(tabBar_, &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);
    applyTabBarStyle();

    auto* tabRow = new QWidget(this);
    tabRow->setObjectName("tabRow");
    tabRow->setAttribute(Qt::WA_StyledBackground, true);
    auto* tabRowLayout = new QHBoxLayout(tabRow);
    tabRowLayout->setContentsMargins(0, 0, 0, 0);
    tabRowLayout->setSpacing(2);
    tabRowLayout->addWidget(tabBar_);
    tabRowLayout->addStretch(1);

    breadcrumb_ = new BreadcrumbWidget(this);
    connect(breadcrumb_, &BreadcrumbWidget::backRequested, this,
            [this] { closeTab(tabBar_->currentIndex()); });
    connect(breadcrumb_, &BreadcrumbWidget::moreOptionsRequested, this,
            &MainWindow::showMoreOptionsMenu);

    findBar_ = new FindBar(this);
    connect(findBar_, &FindBar::queryChanged, this, &MainWindow::applyFindQuery);
    connect(findBar_, &FindBar::nextRequested, this, [this] { findStep(/*backwards=*/false); });
    connect(findBar_, &FindBar::previousRequested, this, [this] { findStep(/*backwards=*/true); });
    connect(findBar_, &FindBar::closeRequested, this, &MainWindow::closeFindBar);
    connect(editor_, &editor::EditorWidget::findResultChanged, findBar_, &FindBar::setMatchInfo);

    welcomeScreen_ = new WelcomeScreen(context_.repository, context_.recentsService, this);
    connect(welcomeScreen_, &WelcomeScreen::newNoteRequested, this,
            [this] { createNote(context_.rootFolder); });
    connect(welcomeScreen_, &WelcomeScreen::noteSelected, this, &MainWindow::openNote);

    contentStack_ = new QStackedWidget(this);
    contentStack_->addWidget(welcomeScreen_); // índice 0: sin notas abiertas
    contentStack_->addWidget(contentSplitter_); // índice 1: editor/preview

    auto* contentContainer = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(tabRow);
    contentLayout->addWidget(breadcrumb_);
    contentLayout->addWidget(findBar_);
    contentLayout->addWidget(contentStack_, 1);

    mainSplitter_ = new QSplitter(this);
    mainSplitter_->addWidget(sidebar_);
    mainSplitter_->addWidget(contentContainer);
    mainSplitter_->setStretchFactor(0, 0);
    mainSplitter_->setStretchFactor(1, 1);
    mainSplitter_->setSizes({260, 880});

    setCentralWidget(mainSplitter_);

    statusSaveLabel_ = new QLabel(this);
    statusCountsLabel_ = new QLabel(this);
    statusFormatLabel_ = new QLabel(tr("Markdown"), this);
    statusBar()->addWidget(statusSaveLabel_);
    statusBar()->addWidget(statusCountsLabel_);
    statusBar()->addPermanentWidget(statusFormatLabel_);
    updateStatusBar();

    connect(editor_, &editor::EditorWidget::contentChanged, this, &MainWindow::updateStatusBar);

    setViewMode(ViewMode::Editor);
}

void MainWindow::setupMenus() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&Archivo"));

    QAction* newNoteAction = fileMenu->addAction(tr("&Nueva nota"), QKeySequence::New);
    connect(newNoteAction, &QAction::triggered, this,
            [this] { createNote(context_.rootFolder); });

    QAction* saveAction = fileMenu->addAction(tr("&Guardar"), QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::flushAutosave);

    QAction* closeTabAction = fileMenu->addAction(tr("&Cerrar pestaña"), QKeySequence(tr("Ctrl+W")));
    connect(closeTabAction, &QAction::triggered, this,
            [this] { closeTab(tabBar_->currentIndex()); });

    QAction* reopenTabAction = fileMenu->addAction(tr("Reabrir pestaña &cerrada"),
                                                    QKeySequence(tr("Ctrl+Shift+T")));
    connect(reopenTabAction, &QAction::triggered, this, &MainWindow::reopenLastClosedTab);

    fileMenu->addSeparator();

    QAction* findAction = fileMenu->addAction(tr("Buscar en la &nota…"));
    findAction->setShortcut(QKeySequence(QKeySequence::Find)); // Ctrl+F
    connect(findAction, &QAction::triggered, this, &MainWindow::openFindBar);

    QAction* searchAction = fileMenu->addAction(tr("&Buscar en todas las notas…"));
    searchAction->setShortcut(QKeySequence(tr("Ctrl+K")));
    connect(searchAction, &QAction::triggered, this, &MainWindow::openSearchDialog);

    fileMenu->addSeparator();

    QAction* exportAction = fileMenu->addAction(tr("&Exportar…"));
    exportAction->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportCurrentNote);

    QAction* insertTagAction = fileMenu->addAction(tr("Insertar &etiqueta…"));
    connect(insertTagAction, &QAction::triggered, this, &MainWindow::insertTag);

    QMenu* viewMenu = menuBar()->addMenu(tr("&Ver"));
    auto* viewGroup = new QActionGroup(this);
    viewGroup->setExclusive(true);

    QAction* editorAction = viewMenu->addAction(tr("&Editor"));
    editorAction->setCheckable(true);
    editorAction->setChecked(true);
    editorAction->setShortcut(QKeySequence(tr("Alt+1")));
    viewGroup->addAction(editorAction);
    connect(editorAction, &QAction::triggered, this, [this] { setViewMode(ViewMode::Editor); });

    QAction* previewAction = viewMenu->addAction(tr("&Vista previa"));
    previewAction->setCheckable(true);
    previewAction->setShortcut(QKeySequence(tr("Alt+2")));
    viewGroup->addAction(previewAction);
    connect(previewAction, &QAction::triggered, this, [this] { setViewMode(ViewMode::Preview); });

    QAction* splitAction = viewMenu->addAction(tr("&Split"));
    splitAction->setCheckable(true);
    splitAction->setShortcut(QKeySequence(tr("Alt+3")));
    viewGroup->addAction(splitAction);
    connect(splitAction, &QAction::triggered, this, [this] { setViewMode(ViewMode::Split); });

    viewMenu->addSeparator();

    QAction* darkModeAction = viewMenu->addAction(tr("Modo &oscuro"));
    darkModeAction->setCheckable(true);
    darkModeAction->setShortcut(QKeySequence(tr("Ctrl+Shift+D")));
    connect(darkModeAction, &QAction::toggled, this, &MainWindow::toggleDarkMode);
    // Dispara toggleDarkMode() vía la señal si estaba activado la última vez.
    darkModeAction->setChecked(context_.settings.getBool("theme.darkMode").value_or(false));

    QAction* collapseSidebarAction = viewMenu->addAction(tr("Colapsar barra &lateral"));
    collapseSidebarAction->setCheckable(true);
    connect(collapseSidebarAction, &QAction::toggled, this,
            [this](bool collapsed) { sidebar_->setCollapsed(collapsed); });
    // El botón "«"/"»" en la propia barra lateral es el control principal;
    // este ítem de menú solo se mantiene sincronizado con él en ambos
    // sentidos, para quien prefiera el menú o un atajo de teclado futuro.
    connect(sidebar_, &SidebarWidget::collapsedChanged, collapseSidebarAction,
            &QAction::setChecked);
    connect(sidebar_, &SidebarWidget::collapsedChanged, this, [this](bool collapsed) {
        context_.settings.setBool("ui.sidebarCollapsed", collapsed);
    });
    // Cambiar el ancho fijo de un hijo no le garantiza a QSplitter que
    // redistribuya el espacio ya mismo: hay que pedírselo explícito en cada
    // paso de la animación, o el área de trabajo no crece/achica con ella.
    connect(sidebar_, &SidebarWidget::widthAnimated, this, [this](int sidebarWidth) {
        mainSplitter_->setSizes({sidebarWidth, qMax(1, mainSplitter_->width() - sidebarWidth)});
    });
    // Restaurar el estado de la sesión anterior sin animar.
    if (context_.settings.getBool("ui.sidebarCollapsed").value_or(false)) {
        sidebar_->setCollapsed(true, /*animated=*/false);
    }

    QAction* wordWrapAction = viewMenu->addAction(tr("Ajuste de &línea"));
    wordWrapAction->setCheckable(true);
    wordWrapAction->setShortcut(QKeySequence(tr("Alt+Z")));
    connect(wordWrapAction, &QAction::toggled, this, [this](bool enabled) {
        editor_->setWordWrapEnabled(enabled);
        context_.settings.setBool("editor.wordWrap", enabled);
    });
    wordWrapAction->setChecked(context_.settings.getBool("editor.wordWrap").value_or(false));

    viewMenu->addSeparator();

    // QKeySequence::ZoomIn es "Ctrl++"; se añade "Ctrl+=" porque en muchos
    // teclados el "+" exige Shift y la gente pulsa la tecla "=" a secas.
    QAction* zoomInAction = viewMenu->addAction(tr("&Acercar"));
    zoomInAction->setShortcuts({QKeySequence(QKeySequence::ZoomIn), QKeySequence(tr("Ctrl+="))});
    connect(zoomInAction, &QAction::triggered, this, [this] { setZoomLevel(zoomLevel_ + 1); });

    QAction* zoomOutAction = viewMenu->addAction(tr("A&lejar"));
    zoomOutAction->setShortcut(QKeySequence(QKeySequence::ZoomOut));
    connect(zoomOutAction, &QAction::triggered, this, [this] { setZoomLevel(zoomLevel_ - 1); });

    QAction* zoomResetAction = viewMenu->addAction(tr("&Restablecer zoom"));
    zoomResetAction->setShortcut(QKeySequence(tr("Ctrl+0")));
    connect(zoomResetAction, &QAction::triggered, this, [this] { setZoomLevel(0); });

    QMenu* accountMenu = menuBar()->addMenu(tr("&Cuenta"));

    QAction* loginAction = accountMenu->addAction(tr("&Iniciar sesión…"));
    connect(loginAction, &QAction::triggered, this, &MainWindow::openSyncLoginDialog);

    QAction* syncAction = accountMenu->addAction(tr("&Sincronizar ahora"));
    // Ctrl+Shift+S es Exportar; F5 es el atajo habitual de "actualizar".
    syncAction->setShortcut(QKeySequence(tr("F5")));
    connect(syncAction, &QAction::triggered, this, &MainWindow::syncNow);
}

void MainWindow::openNote(const core::NoteId& id) {
    auto it = std::find(openNoteIds_.begin(), openNoteIds_.end(), id);
    if (it != openNoteIds_.end()) {
        int index = static_cast<int>(std::distance(openNoteIds_.begin(), it));
        tabBar_->setCurrentIndex(index); // ya abierta: solo cambia de pestaña
        return;
    }

    auto note = context_.repository.find(id);
    QString title = note ? QString::fromStdString(note->title) : tr("(sin título)");

    openNoteIds_.push_back(id);
    int index = tabBar_->addTab(title);
    tabBar_->setTabData(index, QString::fromStdString(id));
    tabBar_->setCurrentIndex(index); // dispara handleTabChanged, que carga el contenido
}

void MainWindow::handleTabChanged(int index) {
    if (index < 0 || index >= static_cast<int>(openNoteIds_.size())) return;
    contentStack_->setCurrentWidget(contentSplitter_);
    loadNote(openNoteIds_[static_cast<std::size_t>(index)]);
    playNoteSwitchAnimation();
}

void MainWindow::closeTab(int index) {
    if (index < 0 || index >= static_cast<int>(openNoteIds_.size())) return;

    core::NoteId closedId = openNoteIds_[static_cast<std::size_t>(index)];
    if (closedId == currentNoteId_) {
        flushAutosave();
    }

    openNoteIds_.erase(openNoteIds_.begin() + index);
    tabBar_->removeTab(index); // puede disparar currentChanged si la activa cambió

    // Si la nota ya no existe (closeTab llamado como parte de deleteNote,
    // que borra el archivo antes de cerrar su pestaña) no tiene sentido
    // ofrecerla para reabrir: Ctrl+Shift+T fallaría en silencio.
    if (context_.repository.find(closedId)) {
        closedNoteIds_.push_back(closedId);
    }

    if (openNoteIds_.empty()) {
        closeFindBar();
        currentNoteId_.clear();
        editor_->clear();
        preview_->setMarkdownSource(QString());
        breadcrumb_->setEmpty();
        statusSaveLabel_->setText(QString());
        editor_->setNoteDirectory(QString());
        preview_->setBaseDirectory(QString());
        updateStatusBar();
        welcomeScreen_->refresh();
        contentStack_->setCurrentWidget(welcomeScreen_);
    }
}

void MainWindow::reopenLastClosedTab() {
    while (!closedNoteIds_.empty()) {
        core::NoteId id = closedNoteIds_.back();
        closedNoteIds_.pop_back();

        bool alreadyOpen = std::find(openNoteIds_.begin(), openNoteIds_.end(), id) !=
                            openNoteIds_.end();
        if (alreadyOpen || !context_.repository.find(id)) continue; // saltear y probar la anterior

        openNote(id);
        return;
    }
}

void MainWindow::loadNote(const core::NoteId& id) {
    flushAutosave(); // no perder cambios de la nota anterior
    autosaveTimer_->stop();

    loadingNote_ = true;
    editor_->setPlainText(QString::fromStdString(context_.noteService.readContent(id)));
    loadingNote_ = false;

    currentNoteId_ = id;
    context_.recentsService.recordOpened(id);
    sidebar_->refresh();
    sidebar_->tree()->setActiveNote(id);

    if (auto note = context_.repository.find(id)) {
        std::filesystem::path relativeFolder =
            std::filesystem::relative(note->path.parent_path(), context_.rootFolder.path);
        QString folderPart = (relativeFolder.empty() || relativeFolder == ".")
                                  ? QString()
                                  : QString::fromStdString(relativeFolder.generic_string()) + " / ";
        breadcrumb_->setPath(folderPart + QString::fromStdString(note->title));
        breadcrumb_->showSavedPulse(
            QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(note->modifiedAt)));
        statusSaveLabel_->setText(tr("Guardado"));

        QString noteDir = QString::fromStdString(note->path.parent_path().string());
        editor_->setNoteDirectory(noteDir);
        preview_->setBaseDirectory(noteDir);
    }

    updateStatusBar();
}

void MainWindow::playNoteSwitchAnimation() {
    contentOpacity_->setOpacity(0.0);

    auto* animation = new QPropertyAnimation(contentOpacity_, "opacity", this);
    animation->setDuration(180);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::createNote(core::Folder targetFolder) {
    NewNoteDialog dialog(context_.repository, context_.folderService, context_.settings,
                          context_.rootFolder, this);
    dialog.preselectFolder(targetFolder);

    if (dialog.exec() != QDialog::Accepted) return;

    try {
        core::Note note = context_.noteService.createNote(dialog.targetFolder(), dialog.title());
        sidebar_->refresh();
        openNote(note.id);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo crear la nota"),
                              QString::fromStdString(ex.what()));
    }
}

void MainWindow::createSubfolder(core::Folder parentFolder) {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nueva subcarpeta"), tr("Nombre:"),
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    try {
        context_.folderService.createSubfolder(parentFolder, name.trimmed().toStdString());
        sidebar_->refresh();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo crear la carpeta"),
                              QString::fromStdString(ex.what()));
    }
}

void MainWindow::scheduleAutosave() {
    if (loadingNote_) return;
    autosaveTimer_->start();
}

void MainWindow::flushAutosave() {
    if (currentNoteId_.empty()) return;
    statusSaveLabel_->setText(tr("Guardando…"));
    context_.noteService.saveContent(currentNoteId_, editor_->toPlainText().toStdString());
    sidebar_->tree()->setActiveNoteDirty(false);
    sidebar_->refreshTags(); // así una etiqueta nueva aparece apenas se guarda, no recién al cambiar de nota
    breadcrumb_->showSavedPulse(QDateTime::currentDateTime());
    statusSaveLabel_->setText(tr("Guardado"));

    int index = tabBar_->currentIndex();
    if (index < 0) return;
    QString text = tabBar_->tabText(index);
    if (text.endsWith(QString::fromUtf8(" ●"))) {
        tabBar_->setTabText(index, text.chopped(2));
    }
}

void MainWindow::updateStatusBar() {
    QString content = editor_->toPlainText();

    int lines = content.isEmpty() ? 0 : static_cast<int>(content.count('\n')) + 1;
    int words = static_cast<int>(
        content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size());
    int chars = content.size();

    statusCountsLabel_->setText(tr("Líneas %1   Palabras %2   Caracteres %3")
                                     .arg(lines)
                                     .arg(words)
                                     .arg(chars));
}

void MainWindow::showMoreOptionsMenu(const QPoint& globalPos) {
    if (currentNoteId_.empty()) return;

    QMenu menu(this);
    QAction* exportAction = menu.addAction(tr("Exportar…"));
    QAction* closeTabAction = menu.addAction(tr("Cerrar pestaña"));
    menu.addSeparator();
    QAction* deleteAction = menu.addAction(tr("Eliminar nota"));

    QAction* chosen = menu.exec(globalPos);
    if (chosen == exportAction) {
        exportCurrentNote();
    } else if (chosen == closeTabAction) {
        closeTab(tabBar_->currentIndex());
    } else if (chosen == deleteAction) {
        deleteCurrentNote();
    }
}

void MainWindow::deleteCurrentNote() {
    if (currentNoteId_.empty()) return;
    deleteNote(currentNoteId_);
}

void MainWindow::renameFolder(core::Folder folder) {
    bool ok = false;
    QString newName =
        QInputDialog::getText(this, tr("Renombrar carpeta"), tr("Nombre:"), QLineEdit::Normal,
                               QString::fromStdString(folder.name), &ok);
    QString trimmed = newName.trimmed();
    if (!ok || trimmed.isEmpty() || trimmed.toStdString() == folder.name) return;

    try {
        context_.folderService.renameFolder(folder, trimmed.toStdString());
        sidebar_->refresh();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo renombrar"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::deleteFolder(core::Folder folder) {
    auto answer = QMessageBox::question(
        this, tr("Eliminar carpeta"),
        tr("¿Eliminar la carpeta \"%1\" y todo su contenido? No se puede deshacer.")
            .arg(QString::fromStdString(folder.name)));
    if (answer != QMessageBox::Yes) return;

    try {
        context_.folderService.deleteFolder(folder);
        sidebar_->refresh();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo eliminar"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::renameNote(const core::NoteId& id) {
    auto note = context_.repository.find(id);
    if (!note) return;

    bool ok = false;
    QString newTitle =
        QInputDialog::getText(this, tr("Renombrar nota"), tr("Título:"), QLineEdit::Normal,
                               QString::fromStdString(note->title), &ok);
    QString trimmed = newTitle.trimmed();
    if (!ok || trimmed.isEmpty() || trimmed.toStdString() == note->title) return;

    try {
        context_.noteService.renameNote(id, trimmed.toStdString());

        auto it = std::find(openNoteIds_.begin(), openNoteIds_.end(), id);
        if (it != openNoteIds_.end()) {
            int index = static_cast<int>(std::distance(openNoteIds_.begin(), it));
            bool wasDirty = tabBar_->tabText(index).endsWith(QString::fromUtf8(" ●"));
            tabBar_->setTabText(index, wasDirty ? trimmed + QString::fromUtf8(" ●") : trimmed);
        }

        if (id == currentNoteId_) {
            loadNote(id); // refresca breadcrumb con el nuevo título/ruta
        } else {
            sidebar_->refresh();
        }
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo renombrar"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::deleteNote(const core::NoteId& id) {
    auto answer = QMessageBox::question(this, tr("Eliminar nota"),
                                         tr("¿Eliminar esta nota? No se puede deshacer."));
    if (answer != QMessageBox::Yes) return;

    try {
        context_.noteService.deleteNote(id);

        auto it = std::find(openNoteIds_.begin(), openNoteIds_.end(), id);
        if (it != openNoteIds_.end()) {
            closeTab(static_cast<int>(std::distance(openNoteIds_.begin(), it)));
        }
        sidebar_->refresh();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo eliminar"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::setViewMode(ViewMode mode) {
    viewMode_ = mode;

    editor_->setVisible(mode == ViewMode::Editor || mode == ViewMode::Split);
    preview_->setVisible(mode == ViewMode::Preview || mode == ViewMode::Split);

    // El divisor entre editor y preview solo se dibuja en modo Split.
    editor_->setProperty("splitMode", mode == ViewMode::Split);
    editor_->style()->unpolish(editor_);
    editor_->style()->polish(editor_);

    if (mode != ViewMode::Editor) {
        updatePreview(editor_->toPlainText());
    }

    // Una búsqueda abierta sigue a la vista que quedó visible.
    if (findBar_ && findBar_->isVisible()) applyFindQuery(findBar_->query());
}

void MainWindow::toggleDarkMode(bool enabled) {
    darkMode_ = enabled;
    qApp->setStyleSheet(theme::stylesheet(darkMode_));
    qApp->setProperty("noctisHoverColor", theme::hoverColor(darkMode_));
    context_.settings.setBool("theme.darkMode", enabled);
    applyTabBarStyle();
    preview_->setDarkMode(darkMode_);
}

void MainWindow::setZoomLevel(int level) {
    constexpr int kMinZoom = -4; // 10pt base -> 6pt
    constexpr int kMaxZoom = 24; // -> 34pt
    zoomLevel_ = qBound(kMinZoom, level, kMaxZoom);
    editor_->setZoomLevel(zoomLevel_);
    preview_->setZoomLevel(zoomLevel_);
    context_.settings.setInt("ui.zoomLevel", zoomLevel_);
}

void MainWindow::applyTabBarStyle() {
    // Reemplaza el ícono nativo de cerrar pestaña por uno recoloreado según
    // el tema (ver TabCloseButtonStyle). Primero se instala el estilo nuevo
    // y recién después se libera el anterior, para que tabBar_ nunca quede
    // apuntando a un QStyle ya destruido.
    auto newStyle = std::make_unique<theme::TabCloseButtonStyle>(darkMode_);
    tabBar_->setStyle(newStyle.get());
    tabBarStyle_ = std::move(newStyle);
}

void MainWindow::insertTag() {
    if (currentNoteId_.empty()) return;

    std::set<std::string> uniqueTags;
    for (const auto& occurrence :
         context_.tagService.collectAll(context_.repository, context_.rootFolder)) {
        uniqueTags.insert(occurrence.tag);
    }

    InsertTagDialog dialog({uniqueTags.begin(), uniqueTags.end()}, this);
    if (dialog.exec() != QDialog::Accepted) return;

    QString tag = dialog.tag();
    if (tag.isEmpty()) return;

    editor_->insertPlainText("#" + tag + " ");
}

void MainWindow::openTagSearch(const QString& tag) {
    SearchDialog dialog(context_.searchService, this, "#" + tag);
    connect(&dialog, &SearchDialog::noteSelected, this, &MainWindow::openNote);
    dialog.exec();
}

void MainWindow::openSettingsDialog() {
    int tabWidth = context_.settings.getInt("editor.tabWidth").value_or(4);
    SettingsDialog dialog(context_.settings, darkMode_, tabWidth,
                           QString::fromStdString(context_.rootFolder.path.string()), this);
    connect(&dialog, &SettingsDialog::darkModeToggled, this, &MainWindow::toggleDarkMode);
    connect(&dialog, &SettingsDialog::tabWidthChanged, this, [this](int spaces) {
        context_.settings.setInt("editor.tabWidth", spaces);
        editor_->setTabWidth(spaces);
    });
    connect(&dialog, &SettingsDialog::notesRootPathChangeRequested, this,
            [this](const QString& path) {
                context_.settings.setString("notes.rootPath", path.toStdString());
                QMessageBox::information(this, tr("Carpeta de notas"),
                                          tr("Reinicia Noctis para terminar de aplicar el cambio."));
            });
    dialog.exec();
}

void MainWindow::openShortcutsDialog() {
    ShortcutsDialog dialog(this);
    dialog.exec();
}

void MainWindow::openHelpDialog() {
    HelpDialog dialog(this);
    dialog.exec();
}

void MainWindow::updatePreview(const QString& content) {
    if (viewMode_ == ViewMode::Editor) return;
    preview_->setMarkdownSource(content);
}

void MainWindow::openSearchDialog() {
    SearchDialog dialog(context_.searchService, this);
    connect(&dialog, &SearchDialog::noteSelected, this, &MainWindow::openNote);
    dialog.exec();
}

void MainWindow::openFindBar() {
    if (currentNoteId_.empty()) return;

    // Como en otros editores: si hay una frase seleccionada en una sola línea,
    // se usa como texto a buscar.
    QString seed;
    if (viewMode_ != ViewMode::Preview) {
        QString selected = editor_->textCursor().selectedText();
        if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator)) seed = selected;
    }
    findBar_->activate(seed);
    applyFindQuery(findBar_->query());
}

void MainWindow::closeFindBar() {
    if (!findBar_->isVisible()) return;
    findBar_->hide();
    editor_->clearFind();
    QTextCursor cleared = preview_->textCursor();
    cleared.clearSelection();
    preview_->setTextCursor(cleared);
    (viewMode_ == ViewMode::Preview ? static_cast<QWidget*>(preview_)
                                    : static_cast<QWidget*>(editor_))->setFocus();
}

void MainWindow::applyFindQuery(const QString& query) {
    if (viewMode_ == ViewMode::Preview) {
        editor_->clearFind();
        findInPreview(query, /*backwards=*/false, /*restart=*/true);
    } else {
        editor_->setFindQuery(query);
    }
}

void MainWindow::findStep(bool backwards) {
    if (findBar_->query().isEmpty()) return;
    if (viewMode_ == ViewMode::Preview) {
        findInPreview(findBar_->query(), backwards, /*restart=*/false);
    } else {
        editor_->findNext(backwards);
    }
}

void MainWindow::findInPreview(const QString& query, bool backwards, bool restart) {
    QTextCursor cursor = preview_->textCursor();
    if (query.isEmpty()) {
        cursor.clearSelection();
        preview_->setTextCursor(cursor);
        findBar_->setMatchInfo(0, 0);
        return;
    }
    if (restart) {
        // Seguir escribiendo no debe saltar a otra coincidencia: se vuelve a
        // buscar desde el inicio de la actual.
        cursor.setPosition(cursor.selectionStart());
        preview_->setTextCursor(cursor);
    }

    const QTextDocument::FindFlags flags =
        backwards ? QTextDocument::FindBackward : QTextDocument::FindFlags();
    bool found = preview_->find(query, flags);
    if (!found) { // dar la vuelta al documento
        QTextCursor wrap = preview_->textCursor();
        wrap.movePosition(backwards ? QTextCursor::End : QTextCursor::Start);
        preview_->setTextCursor(wrap);
        found = preview_->find(query, flags);
    }

    int total = 0;
    QTextCursor match(preview_->document());
    while (!(match = preview_->document()->find(query, match)).isNull()) ++total;
    findBar_->setMatchInfo(0, found ? total : 0);
}

void MainWindow::exportCurrentNote() {
    if (currentNoteId_.empty()) {
        QMessageBox::information(this, tr("Exportar"), tr("Abre una nota primero."));
        return;
    }

    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(
        this, tr("Exportar nota"), QString(),
        tr("Markdown (*.md);;HTML (*.html);;PDF (*.pdf)"), &selectedFilter);
    if (path.isEmpty()) return;

    bool wantsPdf = selectedFilter.contains("pdf", Qt::CaseInsensitive) ||
                    path.endsWith(".pdf", Qt::CaseInsensitive);
    if (wantsPdf) {
        if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";
        exportToPdf(path);
        return;
    }

    core::ExportFormat format = (selectedFilter.contains("html", Qt::CaseInsensitive) ||
                                  path.endsWith(".html", Qt::CaseInsensitive))
                                     ? core::ExportFormat::Html
                                     : core::ExportFormat::Markdown;

    if (!path.endsWith(".md", Qt::CaseInsensitive) &&
        !path.endsWith(".html", Qt::CaseInsensitive)) {
        path += (format == core::ExportFormat::Html) ? ".html" : ".md";
    }

    try {
        context_.exportService.exportNote(editor_->toPlainText().toStdString(),
                                           path.toStdString(), format);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("No se pudo exportar"), QString::fromStdString(ex.what()));
    }
}

void MainWindow::exportToPdf(const QString& path) {
    // El PDF se imprime desde un documento propio (no el de la vista previa):
    // así no depende del estado de la pantalla (zoom, tema oscuro, ancho del
    // panel) ni hace falta cambiar y restaurar la fuente del preview.
    // PDF es un detalle de impresión de Qt, no una responsabilidad del Core.
    std::unique_ptr<QTextDocument> document = preview_->createPrintDocument(editor_->toPlainText());

    QPdfWriter pdfWriter(path);
    pdfWriter.setResolution(300);
    document->print(&pdfWriter);
}

bool MainWindow::ensureSyncService() {
    auto serverUrl = context_.settings.getString("sync.serverUrl");
    if (!serverUrl || serverUrl->empty()) {
        QMessageBox::information(this, tr("Sincronización"),
                                  tr("Primero configura el servidor e inicia sesión, desde "
                                     "el menú Cuenta."));
        return false;
    }

    // El cliente HTTP fija su URL al construirse: si el usuario cambió el
    // servidor desde el diálogo de login, hay que reconstruirlo.
    if (!syncService_ || syncServiceServerUrl_ != *serverUrl) {
        try {
            syncClient_ = std::make_unique<sync::HttpSyncClient>(*serverUrl);
        } catch (const core::SyncError& ex) {
            QMessageBox::warning(this, tr("Sincronización"), QString::fromStdString(ex.what()));
            return false;
        }
        syncService_ = std::make_unique<core::SyncService>(*syncClient_, context_.repository,
                                                             context_.settings, context_.rootFolder);
        syncServiceServerUrl_ = *serverUrl;
    }

    return true;
}

void MainWindow::openSyncLoginDialog() {
    SyncLoginDialog dialog(context_.settings, this);
    if (dialog.exec() != QDialog::Accepted) return;

    context_.settings.setString("sync.serverUrl", dialog.serverUrl());
    if (!ensureSyncService()) return;

    try {
        syncService_->login(dialog.email(), dialog.password(), dialog.deviceName());
    } catch (const core::SyncError& ex) {
        QMessageBox::warning(this, tr("No se pudo iniciar sesión"),
                              QString::fromStdString(ex.what()));
        return;
    }

    QMessageBox::information(this, tr("Sesión iniciada"),
                              tr("Ya puedes sincronizar desde el menú Cuenta."));
}

void MainWindow::syncNow() {
    if (!ensureSyncService()) return;

    // WinHTTP es síncrono: la ventana se congela brevemente durante la
    // llamada. Aceptable para esta primera versión; una futura mejora sería
    // moverlo a un hilo aparte para redes lentas.
    try {
        core::SyncSummary summary = syncService_->syncNow();
        sidebar_->refresh(); // pueden haber llegado notas nuevas o movido a Eliminadas

        QString message = tr("Recibidas: %1 · Enviadas: %2")
                               .arg(summary.pulled)
                               .arg(summary.pushed);
        if (summary.conflicts > 0) {
            message += tr("\n\n%1 nota(s) tenían ediciones en conflicto: se guardó tu versión "
                           "local como copia \"(conflicto)\" junto a la original.")
                           .arg(summary.conflicts);
        }
        QMessageBox::information(this, tr("Sincronización completa"), message);
    } catch (const core::SyncError& ex) {
        QMessageBox::warning(this, tr("No se pudo sincronizar"), QString::fromStdString(ex.what()));
    }
}

} // namespace noctis::ui
