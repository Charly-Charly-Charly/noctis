#pragma once

#include <QMainWindow>

#include <memory>
#include <string>
#include <vector>

#include "UI/AppContext.h"

class QSplitter;
class QTimer;
class QTabBar;
class QGraphicsOpacityEffect;
class QLabel;
class QStackedWidget;

namespace noctis::editor {
class EditorWidget;
}

namespace noctis::sync {
class HttpSyncClient;
}

namespace noctis::core {
class SyncService;
}

namespace noctis::ui::theme {
class TabCloseButtonStyle;
}

namespace noctis::spelling {
class SpellChecker;
}

namespace noctis::ui {

class SidebarWidget;
class BreadcrumbWidget;
class WelcomeScreen;
class PreviewWidget;

// Ventana principal. Solo orquesta widgets y delega toda la lógica a los
// Services del Core (agrupados en AppContext): no contiene reglas de negocio.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(AppContext context, QWidget* parent = nullptr);

    // Declarado explícitamente (y definido en el .cpp): syncClient_ y
    // syncService_ son unique_ptr a tipos que este header solo declara
    // adelantados, así que su destrucción necesita verlos completos.
    ~MainWindow() override;

private:
    enum class ViewMode { Editor, Preview, Split };

    void setupLayout();
    void setupMenus();
    void loadNote(const core::NoteId& id);
    void playNoteSwitchAnimation();

    void openNote(const core::NoteId& id);
    void closeTab(int index);
    void reopenLastClosedTab();
    void handleTabChanged(int index);
    void createNote(core::Folder targetFolder);
    void createSubfolder(core::Folder parentFolder);
    void scheduleAutosave();
    void flushAutosave();
    void setViewMode(ViewMode mode);
    void updatePreview(const QString& content);
    void openSearchDialog();
    void exportCurrentNote();
    void exportToPdf(const QString& path);
    void toggleDarkMode(bool enabled);

    // Crea o recrea syncService_ si aún no existe o si el servidor configurado
    // cambió. Devuelve false (y avisa al usuario) si no hay servidor configurado.
    bool ensureSyncService();
    void openSyncLoginDialog();
    void syncNow();
    void openTagSearch(const QString& tag);
    void openSettingsDialog();
    void openShortcutsDialog();
    void openHelpDialog();
    void insertTag();
    void showMoreOptionsMenu(const QPoint& globalPos);
    void deleteCurrentNote();
    void renameFolder(core::Folder folder);
    void deleteFolder(core::Folder folder);
    void renameNote(const core::NoteId& id);
    void deleteNote(const core::NoteId& id);
    void updateStatusBar();
    void applyTabBarStyle();

    AppContext context_;

    editor::EditorWidget* editor_ = nullptr;
    PreviewWidget* preview_ = nullptr;
    SidebarWidget* sidebar_ = nullptr;
    QSplitter* mainSplitter_ = nullptr;
    QSplitter* contentSplitter_ = nullptr;
    QTimer* autosaveTimer_ = nullptr;

    QTabBar* tabBar_ = nullptr;
    std::unique_ptr<theme::TabCloseButtonStyle> tabBarStyle_;
    BreadcrumbWidget* breadcrumb_ = nullptr;
    QStackedWidget* contentStack_ = nullptr;
    WelcomeScreen* welcomeScreen_ = nullptr;
    QLabel* statusSaveLabel_ = nullptr;
    QLabel* statusCountsLabel_ = nullptr;
    QLabel* statusFormatLabel_ = nullptr;
    QGraphicsOpacityEffect* contentOpacity_ = nullptr;
    std::vector<core::NoteId> openNoteIds_;
    std::vector<core::NoteId> closedNoteIds_; // pila para Ctrl+Shift+T: el último cerrado, primero en volver
    bool changingTabProgrammatically_ = false; // evita reentrancia entre openNote() y handleTabChanged()

    std::unique_ptr<spelling::SpellChecker> spellChecker_;
    std::unique_ptr<sync::HttpSyncClient> syncClient_;
    std::unique_ptr<core::SyncService> syncService_;
    std::string syncServiceServerUrl_; // para saber si hay que recrear syncClient_/syncService_

    core::NoteId currentNoteId_;
    bool loadingNote_ = false; // evita que la carga programática dispare autoguardado
    bool syncingScroll_ = false; // evita eco infinito entre los scroll de editor y preview
    ViewMode viewMode_ = ViewMode::Editor;
    bool darkMode_ = false;
};

} // namespace noctis::ui
