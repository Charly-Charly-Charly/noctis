#include "Utilities/Paths.h"

#include <cstdlib>

namespace noctis::paths {

namespace {
std::filesystem::path homeDirectoryFallback() {
#if defined(_WIN32)
    if (const char* profile = std::getenv("USERPROFILE")) {
        return std::filesystem::path(profile);
    }
    return std::filesystem::current_path();
#else
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home);
    }
    return std::filesystem::current_path();
#endif
}
} // namespace

std::filesystem::path configDirectory() {
#if defined(_WIN32)
    if (const char* appData = std::getenv("APPDATA")) {
        return std::filesystem::path(appData) / "noctis";
    }
    return homeDirectoryFallback() / "noctis";
#elif defined(__APPLE__)
    return homeDirectoryFallback() / "Library" / "Application Support" / "noctis";
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdgConfig) / "noctis";
    }
    return homeDirectoryFallback() / ".config" / "noctis";
#endif
}

std::filesystem::path indexDatabasePath() {
    return configDirectory() / "index.sqlite3";
}

} // namespace noctis::paths
