#pragma once

#include "Core/Ports/ISearchIndex.h"
#include "Database/SqliteConnection.h"

namespace noctis::db {

// Implementación de ISearchIndex sobre una tabla virtual FTS5. Este índice
// es siempre reconstruible desde los .md: si se borra, no se pierde nada.
class SqliteSearchIndex : public core::ISearchIndex {
public:
    explicit SqliteSearchIndex(SqliteConnection& connection);

    std::vector<core::SearchResult> search(const std::string& query) const override;
    void reindex(const core::Note& note, const std::string& content) override;
    void remove(const core::NoteId& id) override;

private:
    void ensureSchema();

    SqliteConnection& connection_;
};

} // namespace noctis::db
