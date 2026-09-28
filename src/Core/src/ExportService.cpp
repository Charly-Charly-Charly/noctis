#include "Core/Services/ExportService.h"

namespace noctis::core {

ExportService::ExportService(IExporter& exporter) : exporter_(exporter) {}

void ExportService::exportNote(const std::string& markdownContent,
                                const std::filesystem::path& destination,
                                ExportFormat format) {
    exporter_.exportNote(markdownContent, destination, format);
}

} // namespace noctis::core
