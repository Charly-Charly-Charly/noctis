#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace noctis::core {

using NoteId = std::string; // UUID estable en el frontmatter del .md, no la ruta

struct Note {
    NoteId id;
    std::string title;
    std::filesystem::path path;      // ruta absoluta real en disco
    std::vector<std::string> tags;    // extraídas del contenido, nunca almacenadas aparte
    bool isFavorite = false;
    std::chrono::system_clock::time_point modifiedAt;
};

} // namespace noctis::core
