#pragma once

#include <string>

#include "Core/Models/Folder.h"
#include "Core/Ports/INoteRepository.h"

namespace noctis::core {

// Las "carpetas" de Noctis son carpetas reales del sistema operativo: este
// servicio delega en el repositorio, que a su vez opera sobre std::filesystem.
class FolderService {
public:
    explicit FolderService(INoteRepository& repository);

    Folder createSubfolder(const Folder& parent, const std::string& name);
    void renameFolder(Folder& folder, const std::string& newName);
    void moveFolder(Folder& folder, const Folder& destination);
    void deleteFolder(const Folder& folder);

private:
    INoteRepository& repository_;
};

} // namespace noctis::core
