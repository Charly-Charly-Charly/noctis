#pragma once

#include <QWidget>

#include <QMap>

#include "Core/Models/Folder.h"
#include "Core/Ports/INoteRepository.h"
#include "Core/Ports/ISettingsStore.h"
#include "Core/Services/FavoritesService.h"
#include "Core/Services/RecentsService.h"
#include "Core/Services/TagService.h"

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace noctis::ui {

class FolderTreeWidget;

// Panel izquierdo completo: cabecera, árbol de notas y etiquetas reales
// (agregadas recorriendo el vault, no un feature separado inventado).
class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    SidebarWidget(core::INoteRepository& repository, core::FavoritesService& favoritesService,
                  core::RecentsService& recentsService, core::TagService& tagService,
                  core::ISettingsStore& settings, core::Folder rootFolder,
                  QWidget* parent = nullptr);

    FolderTreeWidget* tree() const { return tree_; }

    // Reconstruye árbol y etiquetas: tras crear/borrar notas o carpetas.
    void refresh();

    // Solo la lista de etiquetas, sin tocar el árbol (que perdería las
    // carpetas expandidas si se reconstruyera entero): pensado para llamarse
    // después de cada autoguardado, así una etiqueta nueva aparece enseguida
    // en vez de recién cuando algo más dispare un refresh() completo.
    void refreshTags();

    // Colapsada, la barra lateral queda como una mini franja de iconos
    // (nueva nota, ajustes, atajos, ayuda) en vez de desaparecer del todo:
    // siguen siendo accesibles con un solo click, y el botón para volver a
    // agrandarla queda siempre a mano en el header.
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed_; }

signals:
    void collapsedChanged(bool collapsed);
    void noteActivated(const core::NoteId& id);
    void createNoteRequested(const core::Folder& targetFolder);
    void createSubfolderRequested(const core::Folder& parentFolder);
    void renameFolderRequested(const core::Folder& folder);
    void deleteFolderRequested(const core::Folder& folder);
    void renameNoteRequested(const core::NoteId& id);
    void deleteNoteRequested(const core::NoteId& id);
    void tagActivated(const QString& tag);
    void settingsRequested();
    void shortcutsRequested();
    void helpRequested();

private:
    void rebuildTags();

    core::INoteRepository& repository_;
    core::TagService& tagService_;
    core::ISettingsStore& settings_;
    core::Folder rootFolder_;

    FolderTreeWidget* tree_ = nullptr;
    QVBoxLayout* tagsLayout_ = nullptr;

    // Título y "+" del header se esconden al colapsar (collapseButton_ no:
    // es lo único que queda visible ahí para poder volver a expandir).
    QLabel* titleLabel_ = nullptr;
    QLabel* starLabel_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* collapseButton_ = nullptr;

    // Contenido normal (árbol, etiquetas) vs. mini franja de iconos: se
    // togglean como bloque, uno visible por vez.
    QWidget* expandedContent_ = nullptr;
    QWidget* collapsedRail_ = nullptr;

    QLabel* spacesLabel_ = nullptr;
    QPushButton* newSpaceButton_ = nullptr;
    QLabel* tagsLabel_ = nullptr;
    QWidget* tagsContainer_ = nullptr;
    QWidget* iconRowContainer_ = nullptr;
    bool collapsed_ = false;
};

} // namespace noctis::ui
