#pragma once

#include <QTreeWidget>

#include "Core/Models/Folder.h"
#include "Core/Ports/INoteRepository.h"
#include "Core/Services/FavoritesService.h"
#include "Core/Services/RecentsService.h"

namespace noctis::ui {

// Árbol de navegación: Favoritos, Recientes, y la jerarquía real de
// carpetas/notas bajo la raíz. Las carpetas son directorios reales del SO;
// este widget solo las refleja, nunca las reinterpreta.
//
// Cada fila se numera según su posición entre sus hermanos (001, 002…), como
// referencia rápida de "dónde estoy" independiente del nombre.
class FolderTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    FolderTreeWidget(core::INoteRepository& repository, core::FavoritesService& favoritesService,
                      core::RecentsService& recentsService, core::Folder rootFolder,
                      QWidget* parent = nullptr);

    void refresh();

    // La nota "activa" no es la selección nativa del árbol (se pierde con
    // cualquier click fuera de él): es la que MainWindow tiene abierta.
    void setActiveNote(const core::NoteId& id);
    void setActiveNoteDirty(bool dirty);

signals:
    void noteActivated(const core::NoteId& id);
    void createNoteRequested(const core::Folder& targetFolder);
    void createSubfolderRequested(const core::Folder& parentFolder);
    void renameFolderRequested(const core::Folder& folder);
    void deleteFolderRequested(const core::Folder& folder);
    void renameNoteRequested(const core::NoteId& id);
    void deleteNoteRequested(const core::NoteId& id);

private:
    QTreeWidgetItem* populateFolderItem(QTreeWidgetItem* parentItem, const core::Folder& folder,
                                         int number);
    void populateSpecialSections();
    void showFolderContextMenu(const core::Folder& folder, const QPoint& globalPos);
    void showNoteContextMenu(const core::NoteId& id, const QPoint& globalPos);

    QTreeWidgetItem* makeSectionItem(QTreeWidgetItem* parent, int number, const QString& icon,
                                      const QString& title);
    QTreeWidgetItem* makeNoteItem(QTreeWidgetItem* parent, int number, const core::Note& note);

    core::INoteRepository& repository_;
    core::FavoritesService& favoritesService_;
    core::RecentsService& recentsService_;
    core::Folder rootFolder_;

    core::NoteId activeNoteId_;
    QTreeWidgetItem* activeItem_ = nullptr;
};

} // namespace noctis::ui
