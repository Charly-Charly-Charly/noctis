#include "Utilities/Logger.h"

#include <cstdio>

namespace noctis {

namespace {
const char* levelLabel(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "?";
}
} // namespace

void Logger::log(LogLevel level, std::string_view message) {
    std::fprintf(stderr, "[%s] %.*s\n", levelLabel(level),
                 static_cast<int>(message.size()), message.data());
}

} // namespace noctis
