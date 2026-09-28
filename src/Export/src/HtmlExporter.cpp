#include "Export/HtmlExporter.h"

#include "Markdown/HtmlRenderer.h"

#include <fstream>
#include <stdexcept>

namespace noctis::exportmod {

void HtmlExporter::exportNote(const std::string& markdownContent,
                               const std::filesystem::path& destination,
                               core::ExportFormat format) {
    switch (format) {
        case core::ExportFormat::Markdown:
            exportAsMarkdown(markdownContent, destination);
            break;
        case core::ExportFormat::Html:
            exportAsHtml(markdownContent, destination);
            break;
        case core::ExportFormat::Pdf:
            // TODO: renderizar el HTML resultante a PDF (p. ej. vía QPrinter
            // en la capa UI, que sí puede depender de Qt).
            throw std::runtime_error("Exportación a PDF todavía no implementada");
    }
}

void HtmlExporter::exportAsMarkdown(const std::string& markdownContent,
                                     const std::filesystem::path& destination) {
    std::ofstream file(destination, std::ios::binary);
    file << markdownContent;
}

void HtmlExporter::exportAsHtml(const std::string& markdownContent,
                                 const std::filesystem::path& destination) {
    md::HtmlRenderer renderer;
    std::string html = renderer.render(markdownContent);

    std::ofstream file(destination, std::ios::binary);
    file << "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"></head><body>\n"
         << html << "\n</body></html>\n";
}

} // namespace noctis::exportmod
