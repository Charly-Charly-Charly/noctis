#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Core/Models/Folder.h"
#include "Core/Models/Note.h"

namespace noctis::core {

// Puerto implementado por Filesystem/. El Core nunca toca std::filesystem
// directamente: toda operación sobre notas y carpetas pasa por aquí. Las
// carpetas de Noctis son carpetas reales del SO, así que este mismo puerto
// cubre ambas responsabilidades en vez de fragmentarlas artificialmente.
class INoteRepository {
public:
    virtual ~INoteRepository() = default;

    virtual std::optional<Note> find(const NoteId& id) const = 0;
    virtual std::vector<Note> listInFolder(const Folder& folder) const = 0;

    virtual std::string readContent(const NoteId& id) const = 0;
    virtual void writeContent(const NoteId& id, const std::string& content) = 0;

    virtual Note create(const Folder& folder, const std::string& title) = 0;

    // Materializa en disco una nota cuya identidad ya viene de otro lado (el
    // servidor de sync): a diferencia de create(), no genera un id nuevo,
    // recibe el que ya existe. Es lo único que permite que dos dispositivos
    // terminen viendo la misma nota como la misma nota.
    virtual Note importRemote(const NoteId& id, const Folder& folder, const std::string& title,
                               const std::string& content) = 0;

    virtual void rename(const NoteId& id, const std::string& newTitle) = 0;
    virtual void move(const NoteId& id, const Folder& destination) = 0;
    virtual void remove(const NoteId& id) = 0;

    virtual bool existsInFolder(const Folder& folder, const std::string& title) const = 0;

    virtual std::vector<Folder> listSubfolders(const Folder& parent) const = 0;
    virtual Folder createFolder(const Folder& parent, const std::string& name) = 0;
    virtual void renameFolder(const Folder& folder, const std::string& newName) = 0;
    virtual void moveFolder(const Folder& folder, const Folder& destination) = 0;
    virtual void removeFolder(const Folder& folder) = 0;
};

} // namespace noctis::core
