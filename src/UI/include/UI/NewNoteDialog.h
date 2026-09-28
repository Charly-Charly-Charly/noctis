#pragma once

#include <QDialog>

#include <string>

#include "Core/Models/Folder.h"
#include "Core/Ports/INoteRepository.h"
#include "Core/Ports/ISettingsStore.h"
#include "Core/Services/FolderService.h"

class QComboBox;
class QLineEdit;
class QLabel;

namespace noctis::ui {

// Formulario de creación de notas: título + carpeta + subcarpeta, con
// vista previa de la ruta final y creación de carpetas sin salir del
// diálogo. Recuerda la última ubicación usada (vía Settings).
class NewNoteDialog : public QDialog {
    Q_OBJECT

public:
    NewNoteDialog(core::INoteRepository& repository, core::FolderService& folderService,
                  core::ISettingsStore& settings, core::Folder rootFolder,
                  QWidget* parent = nullptr);

    // Preselecciona una carpeta (p. ej. al crear desde el menú contextual del árbol).
    void preselectFolder(const core::Folder& folder);

    std::string title() const;
    core::Folder targetFolder() const;

    void accept() override;

private:
    void handleFolderChanged();
    void handleCreateFolder();
    void handleCreateSubfolder();
    void updatePathPreview();

    void reloadTopFolders(const std::string& preferredName = {});
    void reloadSubfolders(const std::string& preferredName = {});
    core::Folder currentParentFolder() const;

    core::INoteRepository& repository_;
    core::FolderService& folderService_;
    core::ISettingsStore& settings_;
    core::Folder rootFolder_;

    QLineEdit* titleEdit_;
    QComboBox* folderCombo_;
    QComboBox* subfolderCombo_;
    QLabel* pathPreviewLabel_;
};

} // namespace noctis::ui
