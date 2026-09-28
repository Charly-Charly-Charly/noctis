#pragma once

#include <filesystem>
#include <string>

struct sqlite3;

namespace noctis::db {

// Envoltorio RAII mínimo sobre el C API de SQLite. Nada de negocio vive
// aquí: solo abrir/cerrar la conexión y ejecutar sentencias sueltas.
class SqliteConnection {
public:
    explicit SqliteConnection(const std::filesystem::path& databasePath);
    ~SqliteConnection();

    SqliteConnection(const SqliteConnection&) = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;

    void execute(const std::string& sql);

    sqlite3* handle() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

} // namespace noctis::db
