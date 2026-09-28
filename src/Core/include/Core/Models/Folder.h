#pragma once

#include <filesystem>
#include <string>

namespace noctis::core {

struct Folder {
    std::string name;
    std::filesystem::path path; // ruta absoluta real; es literalmente la carpeta del SO
};

} // namespace noctis::core
