#include "UI/FolderTreeWidget.h"

#include <QAction>
#include <QMenu>
#include <QTreeWidgetItemIterator>

#include <vector>

#include "UI/TreeRowWidget.h"

namespace noctis::ui {

namespace {
constexpr int kKindRole = Qt::UserRole;
constexpr int kPayloadRole = Qt::UserRole + 1;

enum ItemKind { FolderKind = 1, NoteKind = 2, SectionKind = 3 };

QString numberOf(int position) {
    return QString("%1").arg(position, 3, 10, QChar('0'));
}
} // namespace

FolderTreeWidget::FolderTreeWidget(core::INoteRepository& repository,
                                    core::FavoritesService& favoritesService,
                                    core::RecentsService& recentsService, core::Folder rootFolder,
                                    QWidget* parent)
    : QTreeWidget(parent),
      repository_(repository),
      favoritesService_(favoritesService),
      recentsService_(recentsService),
      rootFolder_(std::move(rootFolder)) {
    setHeaderHidden(true);
    setIndentation(14);
    setAnimated(true); // las carpetas se despliegan/pliegan con transición
    setContextMenuPolicy(Qt::CustomContextMenu);

    refresh();
}

QTreeWidgetItem* FolderTreeWidget::makeSectionItem(QTreeWidgetItem* parent, int number,
                                                    const QString& icon, const QString& title) {
    auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(this);
    item->setData(0, kKindRole, SectionKind);

    auto* row = new TreeRowWidget(numberOf(number), icon, title, TreeRowWidget::Kind::Section);
    connect(row, &TreeRowWidget::activated, this,
            [this, item] { item->setExpanded(!item->isExpanded()); });
    setItemWidget(item, 0, row);
    return item;
}

QTreeWidgetItem* FolderTreeWidget::makeNoteItem(QTreeWidgetItem* parent, int number,
                                                 const core::Note& note) {
    auto* item = new QTreeWidgetItem(parent);
    item->setData(0, kKindRole, NoteKind);
    item->setData(0, kPayloadRole, QString::fromStdString(note.id));

    auto* row = new TreeRowWidget(numberOf(number), QString(), QString::fromStdString(note.title),
                                   TreeRowWidget::Kind::Note);
    connect(row, &TreeRowWidget::activated, this,
            [this, id = note.id] { emit noteActivated(id); });
    connect(row, &TreeRowWidget::contextMenuRequested, this,
            [this, id = note.id](const QPoint& pos) { showNoteContextMenu(id, pos); });
    setItemWidget(item, 0, row);

    if (note.id == activeNoteId_) activeItem_ = item;

    return item;
}

void FolderTreeWidget::refresh() {
    clear();
    activeItem_ = nullptr;

    populateSpecialSections();

    int position = 3; // Favoritos y Recientes ya ocuparon 001 y 002
    for (const core::Folder& folder : repository_.listSubfolders(rootFolder_)) {
        auto* item = new QTreeWidgetItem(this);
        item->setData(0, kKindRole, FolderKind);
        item->setData(0, kPayloadRole, QString::fromStdString(folder.path.string()));

        auto* row = new TreeRowWidget(numberOf(position), QStringLiteral("folder"),
                                       QString::fromStdString(folder.name),
                                       TreeRowWidget::Kind::Folder);
        connect(row, &TreeRowWidget::activated, this,
                [item] { item->setExpanded(!item->isExpanded()); });
        connect(row, &TreeRowWidget::contextMenuRequested, this,
                [this, folder](const QPoint& pos) { showFolderContextMenu(folder, pos); });
        setItemWidget(item, 0, row);

        populateFolderItem(item, folder, 1);
        ++position;
    }

    if (activeItem_) {
        // La fila puede haberse recreado (refresh() reconstruye todo el
        // árbol): sin esto, el resaltado de "nota activa" desaparecería cada
        // vez que se abre o guarda una nota.
        if (auto* row = qobject_cast<TreeRowWidget*>(itemWidget(activeItem_, 0))) {
            row->setActive(true);
        }
    }
}

void FolderTreeWidget::populateSpecialSections() {
    auto* favoritesSection = makeSectionItem(nullptr, 1, QStringLiteral("star"), tr("Favoritos"));
    int favPos = 1;
    for (const auto& id : favoritesService_.listFavorites()) {
        if (auto note = repository_.find(id)) makeNoteItem(favoritesSection, favPos++, *note);
    }

    auto* recentsSection = makeSectionItem(nullptr, 2, QStringLiteral("history"), tr("Recientes"));
    int recPos = 1;
    for (const auto& id : recentsService_.listRecents()) {
        if (auto note = repository_.find(id)) makeNoteItem(recentsSection, recPos++, *note);
    }
}

QTreeWidgetItem* FolderTreeWidget::populateFolderItem(QTreeWidgetItem* parentItem,
                                                       const core::Folder& folder, int number) {
    int position = number;

    for (const core::Folder& subfolder : repository_.listSubfolders(folder)) {
        auto* item = new QTreeWidgetItem(parentItem);
        item->setData(0, kKindRole, FolderKind);
        item->setData(0, kPayloadRole, QString::fromStdString(subfolder.path.string()));

        auto* row = new TreeRowWidget(numberOf(position), QStringLiteral("folder"),
                                       QString::fromStdString(subfolder.name),
                                       TreeRowWidget::Kind::Folder);
        connect(row, &TreeRowWidget::activated, this,
                [item] { item->setExpanded(!item->isExpanded()); });
        connect(row, &TreeRowWidget::contextMenuRequested, this,
                [this, subfolder](const QPoint& pos) { showFolderContextMenu(subfolder, pos); });
        setItemWidget(item, 0, row);

        populateFolderItem(item, subfolder, 1);
        ++position;
    }

    for (const core::Note& note : repository_.listInFolder(folder)) {
        makeNoteItem(parentItem, position, note);
        ++position;
    }

    return parentItem;
}

void FolderTreeWidget::setActiveNote(const core::NoteId& id) {
    activeNoteId_ = id;

    if (activeItem_) {
        if (auto* row = qobject_cast<TreeRowWidget*>(itemWidget(activeItem_, 0))) {
            row->setActive(false);
            row->setDirty(false);
        }
    }

    activeItem_ = nullptr;

    // Recorre todo el árbol buscando el item cuyo payload sea este id: es
    // más simple que mantener un índice paralelo, y el árbol es chico.
    for (auto it = QTreeWidgetItemIterator(this); *it; ++it) {
        QTreeWidgetItem* item = *it;
        if (item->data(0, kKindRole).toInt() != NoteKind) continue;
        if (item->data(0, kPayloadRole).toString().toStdString() != id) continue;

        activeItem_ = item;
        if (auto* row = qobject_cast<TreeRowWidget*>(itemWidget(item, 0))) {
            row->setActive(true);
        }
        break;
    }
}

void FolderTreeWidget::setActiveNoteDirty(bool dirty) {
    if (!activeItem_) return;
    if (auto* row = qobject_cast<TreeRowWidget*>(itemWidget(activeItem_, 0))) {
        row->setDirty(dirty);
    }
}

void FolderTreeWidget::showFolderContextMenu(const core::Folder& folder, const QPoint& globalPos) {
    QMenu menu(this);
    QAction* newNoteAction = menu.addAction(tr("Nueva nota aquí"));
    QAction* newSubfolderAction = menu.addAction(tr("Nueva subcarpeta"));
    menu.addSeparator();
    QAction* renameAction = menu.addAction(tr("Renombrar"));
    QAction* deleteAction = menu.addAction(tr("Eliminar carpeta"));

    QAction* chosen = menu.exec(globalPos);
    if (chosen == newNoteAction) {
        emit createNoteRequested(folder);
    } else if (chosen == newSubfolderAction) {
        emit createSubfolderRequested(folder);
    } else if (chosen == renameAction) {
        emit renameFolderRequested(folder);
    } else if (chosen == deleteAction) {
        emit deleteFolderRequested(folder);
    }
}

void FolderTreeWidget::showNoteContextMenu(const core::NoteId& id, const QPoint& globalPos) {
    bool isFavorite = favoritesService_.isFavorite(id);

    QMenu menu(this);
    QAction* toggleFavoriteAction =
        menu.addAction(isFavorite ? tr("Quitar de favoritos") : tr("Marcar como favorita"));
    menu.addSeparator();
    QAction* renameAction = menu.addAction(tr("Renombrar"));
    QAction* deleteAction = menu.addAction(tr("Eliminar nota"));

    QAction* chosen = menu.exec(globalPos);
    if (chosen == toggleFavoriteAction) {
        favoritesService_.toggleFavorite(id);
        refresh();
    } else if (chosen == renameAction) {
        emit renameNoteRequested(id);
    } else if (chosen == deleteAction) {
        emit deleteNoteRequested(id);
    }
}

} // namespace noctis::ui
