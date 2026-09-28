#pragma once

#include "Core/Ports/IExporter.h"

namespace noctis::exportmod {

// Exporta a HTML directamente vía md4c-html. El formato Markdown es un
// passthrough (guarda tal cual); PDF queda pendiente (ver TODO en el .cpp).
class HtmlExporter : public core::IExporter {
public:
    void exportNote(const std::string& markdownContent,
                     const std::filesystem::path& destination,
                     core::ExportFormat format) override;

private:
    void exportAsHtml(const std::string& markdownContent,
                       const std::filesystem::path& destination);
    void exportAsMarkdown(const std::string& markdownContent,
                           const std::filesystem::path& destination);
};

} // namespace noctis::exportmod
