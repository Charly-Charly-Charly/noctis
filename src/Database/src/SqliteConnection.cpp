#include "Database/SqliteConnection.h"

#include <sqlite3.h>

#include <stdexcept>

namespace noctis::db {

SqliteConnection::SqliteConnection(const std::filesystem::path& databasePath) {
    std::filesystem::create_directories(databasePath.parent_path());

    if (sqlite3_open(databasePath.string().c_str(), &db_) != SQLITE_OK) {
        std::string message = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error("No se pudo abrir la base de datos de índices: " + message);
    }
}

SqliteConnection::~SqliteConnection() {
    if (db_) sqlite3_close(db_);
}

void SqliteConnection::execute(const std::string& sql) {
    char* errorMessage = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        std::string message = errorMessage ? errorMessage : "error desconocido";
        sqlite3_free(errorMessage);
        throw std::runtime_error("Error SQL: " + message);
    }
}

} // namespace noctis::db
