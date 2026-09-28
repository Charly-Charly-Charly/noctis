#pragma once

#include <string_view>

namespace noctis {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
};

class Logger {
public:
    static void log(LogLevel level, std::string_view message);

    static void debug(std::string_view message) { log(LogLevel::Debug, message); }
    static void info(std::string_view message) { log(LogLevel::Info, message); }
    static void warning(std::string_view message) { log(LogLevel::Warning, message); }
    static void error(std::string_view message) { log(LogLevel::Error, message); }
};

} // namespace noctis
