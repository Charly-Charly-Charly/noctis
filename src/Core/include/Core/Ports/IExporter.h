#pragma once

#include <filesystem>
#include <string>

namespace noctis::core {

enum class ExportFormat {
    Markdown,
    Html,
    Pdf,
};

// Puerto implementado por Export/. Recibe markdown ya resuelto y lo
// convierte al formato pedido, preservando el formato original.
class IExporter {
public:
    virtual ~IExporter() = default;

    virtual void exportNote(const std::string& markdownContent,
                             const std::filesystem::path& destination,
                             ExportFormat format) = 0;
};

} // namespace noctis::core
