#include "Core/Services/FolderService.h"

namespace noctis::core {

FolderService::FolderService(INoteRepository& repository) : repository_(repository) {}

Folder FolderService::createSubfolder(const Folder& parent, const std::string& name) {
    return repository_.createFolder(parent, name);
}

void FolderService::renameFolder(Folder& folder, const std::string& newName) {
    repository_.renameFolder(folder, newName);
    folder.name = newName;
}

void FolderService::moveFolder(Folder& folder, const Folder& destination) {
    repository_.moveFolder(folder, destination);
    folder.path = destination.path / folder.name;
}

void FolderService::deleteFolder(const Folder& folder) {
    repository_.removeFolder(folder);
}

} // namespace noctis::core
