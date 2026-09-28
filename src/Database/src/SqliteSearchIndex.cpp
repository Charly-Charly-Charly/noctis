#include "Database/SqliteSearchIndex.h"

#include <sqlite3.h>

#include <cctype>
#include <stdexcept>

namespace noctis::db {

using core::Note;
using core::NoteId;
using core::SearchResult;

namespace {
// FTS5 tiene su propia mini sintaxis de consulta (comillas, guion como NOT,
// dos puntos, paréntesis, asterisco...): pasarle texto arbitrario del
// usuario tal cual puede tirar un error de sintaxis y quedarse sin
// resultados en silencio (pasaba, por ejemplo, buscando "#etiqueta" desde el
// click en una etiqueta del sidebar). Se parte en palabras alfanuméricas y
// cada una se envuelve en comillas para que FTS5 la trate siempre como una
// frase literal, nunca como un operador.
std::string sanitizeFts5Query(const std::string& raw) {
    std::string result;
    std::string word;

    auto flush = [&] {
        if (word.empty()) return;
        if (!result.empty()) result += ' ';
        result += '"' + word + '"';
        word.clear();
    };

    for (unsigned char c : raw) {
        // >= 0x80: bytes de continuación UTF-8 (tildes, ñ) — nunca coinciden
        // con puntuación ASCII, así que se pueden dejar pasar tal cual.
        if (std::isalnum(c) || c >= 0x80) {
            word += static_cast<char>(c);
        } else {
            flush();
        }
    }
    flush();

    return result;
}
} // namespace

SqliteSearchIndex::SqliteSearchIndex(SqliteConnection& connection) : connection_(connection) {
    ensureSchema();
}

void SqliteSearchIndex::ensureSchema() {
    connection_.execute(
        "CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5("
        "  note_id UNINDEXED,"
        "  title,"
        "  content,"
        "  tags"
        ");");
}

void SqliteSearchIndex::reindex(const Note& note, const std::string& content) {
    sqlite3_stmt* stmt = nullptr;

    const char* deleteSql = "DELETE FROM notes_fts WHERE note_id = ?;";
    sqlite3_prepare_v2(connection_.handle(), deleteSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, note.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    std::string tags;
    for (const auto& tag : note.tags) {
        tags += tag + " ";
    }

    const char* insertSql =
        "INSERT INTO notes_fts (note_id, title, content, tags) VALUES (?, ?, ?, ?);";
    sqlite3_prepare_v2(connection_.handle(), insertSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, note.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, note.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, tags.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("No se pudo indexar la nota: " + note.id);
    }
    sqlite3_finalize(stmt);
}

void SqliteSearchIndex::remove(const NoteId& id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM notes_fts WHERE note_id = ?;";
    sqlite3_prepare_v2(connection_.handle(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<SearchResult> SqliteSearchIndex::search(const std::string& query) const {
    std::vector<SearchResult> results;

    std::string sanitized = sanitizeFts5Query(query);
    if (sanitized.empty()) return results;

    const char* sql =
        "SELECT note_id, title, snippet(notes_fts, 2, '', '', '...', 8) "
        "FROM notes_fts WHERE notes_fts MATCH ? ORDER BY rank LIMIT 50;";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(connection_.handle(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, sanitized.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SearchResult result;
        result.noteId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        result.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.snippet = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        results.push_back(std::move(result));
    }

    sqlite3_finalize(stmt);
    return results;
}

} // namespace noctis::db
