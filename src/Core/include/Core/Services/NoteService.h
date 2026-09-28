#pragma once

#include <string>

#include "Core/Models/Folder.h"
#include "Core/Models/Note.h"
#include "Core/Ports/INoteRepository.h"
#include "Core/Ports/ISearchIndex.h"
#include "Core/Services/TagService.h"

namespace noctis::core {

// Caso de uso: crear, leer, modificar y borrar notas. Coordina el
// repositorio (fuente de verdad, el .md) con el índice de búsqueda
// (derivado, reconstruible). El orden importa: primero se escribe el
// archivo, luego se reindexa.
class NoteService {
public:
    NoteService(INoteRepository& repository, ISearchIndex& searchIndex);

    Note createNote(const Folder& folder, const std::string& title);
    void renameNote(const NoteId& id, const std::string& newTitle);
    void moveNote(const NoteId& id, const Folder& destination);
    void deleteNote(const NoteId& id);

    std::string readContent(const NoteId& id) const;
    void saveContent(const NoteId& id, const std::string& content);

private:
    INoteRepository& repository_;
    ISearchIndex& searchIndex_;
    TagService tagService_; // sin estado ni dependencias externas: se instancia directamente
};

} // namespace noctis::core
