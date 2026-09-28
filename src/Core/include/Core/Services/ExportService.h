#pragma once

#include <filesystem>
#include <string>

#include "Core/Ports/IExporter.h"

namespace noctis::core {

class ExportService {
public:
    explicit ExportService(IExporter& exporter);

    void exportNote(const std::string& markdownContent,
                     const std::filesystem::path& destination,
                     ExportFormat format);

private:
    IExporter& exporter_;
};

} // namespace noctis::core
