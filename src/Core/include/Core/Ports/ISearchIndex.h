#pragma once

#include <string>
#include <vector>

#include "Core/Models/Note.h"

namespace noctis::core {

struct SearchResult {
    NoteId noteId;
    std::string title;
    std::string snippet;
};

// Puerto implementado por Database/ (SQLite FTS5). El índice es siempre
// derivado: se puede borrar y reconstruir desde los .md sin pérdida de datos.
class ISearchIndex {
public:
    virtual ~ISearchIndex() = default;

    virtual std::vector<SearchResult> search(const std::string& query) const = 0;
    virtual void reindex(const Note& note, const std::string& content) = 0;
    virtual void remove(const NoteId& id) = 0;
};

} // namespace noctis::core
