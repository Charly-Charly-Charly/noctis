#pragma once

#include <filesystem>

namespace noctis::paths {

// Directorio de configuración del usuario (p. ej. %APPDATA%/noctis en Windows,
// ~/.config/noctis en Linux, ~/Library/Application Support/noctis en macOS).
std::filesystem::path configDirectory();

// Ruta de la base de datos de índices (SQLite). Nunca contiene las notas en
// sí: esas siempre están donde el usuario las puso, en formato .md plano.
std::filesystem::path indexDatabasePath();

} // namespace noctis::paths
