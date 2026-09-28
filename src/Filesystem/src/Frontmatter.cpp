#include "Filesystem/Frontmatter.h"

#include <sstream>
#include <vector>

namespace noctis::fs {

namespace {

constexpr const char* kDelimiter = "---";

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Un delimitador válido es una línea que solo contiene "---".
bool isDelimiter(const std::string& line) {
    return trim(line) == kDelimiter;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace

Document splitDocument(const std::string& raw) {
    std::vector<std::string> lines = splitLines(raw);

    if (lines.empty() || !isDelimiter(lines.front())) {
        return Document{"", raw};
    }

    // Buscamos el cierre del bloque. Sin cierre, no hay frontmatter válido y
    // el "---" inicial es simplemente contenido del usuario.
    std::size_t closing = 0;
    for (std::size_t i = 1; i < lines.size(); ++i) {
        if (isDelimiter(lines[i])) {
            closing = i;
            break;
        }
    }

    if (closing == 0) {
        return Document{"", raw};
    }

    std::string frontmatter;
    for (std::size_t i = 1; i < closing; ++i) {
        frontmatter += lines[i];
        frontmatter += "\n";
    }

    std::string body;
    for (std::size_t i = closing + 1; i < lines.size(); ++i) {
        body += lines[i];
        if (i + 1 < lines.size()) body += "\n";
    }

    // Preservamos el salto final del archivo original si lo tenía.
    if (!raw.empty() && raw.back() == '\n' && !body.empty()) {
        body += "\n";
    }

    return Document{frontmatter, body};
}

std::string joinDocument(const Document& document) {
    if (document.frontmatter.empty()) return document.body;

    std::string result = std::string(kDelimiter) + "\n" + document.frontmatter;
    if (result.back() != '\n') result += "\n";
    result += std::string(kDelimiter) + "\n";
    result += document.body;
    return result;
}

std::optional<std::string> frontmatterField(const std::string& frontmatter,
                                             const std::string& key) {
    for (const std::string& line : splitLines(frontmatter)) {
        const auto separator = line.find(':');
        if (separator == std::string::npos) continue;
        if (trim(line.substr(0, separator)) != key) continue;
        return trim(line.substr(separator + 1));
    }
    return std::nullopt;
}

std::string upsertFrontmatterField(const std::string& frontmatter, const std::string& key,
                                    const std::string& value) {
    std::vector<std::string> lines = splitLines(frontmatter);
    const std::string replacement = key + ": " + value;

    for (std::string& line : lines) {
        const auto separator = line.find(':');
        if (separator == std::string::npos) continue;
        if (trim(line.substr(0, separator)) != key) continue;

        line = replacement;
        std::string result;
        for (const std::string& current : lines) {
            result += current;
            result += "\n";
        }
        return result;
    }

    std::string result;
    for (const std::string& line : lines) {
        result += line;
        result += "\n";
    }
    result += replacement;
    result += "\n";
    return result;
}

} // namespace noctis::fs
