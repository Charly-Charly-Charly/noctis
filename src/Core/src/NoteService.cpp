#include "Core/Services/NoteService.h"

#include <stdexcept>

#include "Utilities/Logger.h"

namespace noctis::core {

NoteService::NoteService(INoteRepository& repository, ISearchIndex& searchIndex)
    : repository_(repository), searchIndex_(searchIndex) {}

Note NoteService::createNote(const Folder& folder, const std::string& title) {
    if (repository_.existsInFolder(folder, title)) {
        throw std::runtime_error("Ya existe una nota con ese nombre en esta carpeta");
    }

    Note note = repository_.create(folder, title);
    std::string content = repository_.readContent(note.id);
    note.tags = tagService_.extractTags(content);
    searchIndex_.reindex(note, content);

    Logger::info("Nota creada: " + note.id);
    return note;
}

void NoteService::renameNote(const NoteId& id, const std::string& newTitle) {
    repository_.rename(id, newTitle);
}

void NoteService::moveNote(const NoteId& id, const Folder& destination) {
    repository_.move(id, destination);
}

void NoteService::deleteNote(const NoteId& id) {
    repository_.remove(id);
    searchIndex_.remove(id);
}

std::string NoteService::readContent(const NoteId& id) const {
    return repository_.readContent(id);
}

void NoteService::saveContent(const NoteId& id, const std::string& content) {
    repository_.writeContent(id, content);

    if (auto note = repository_.find(id)) {
        note->tags = tagService_.extractTags(content);
        searchIndex_.reindex(*note, content);
    }
}

} // namespace noctis::core
