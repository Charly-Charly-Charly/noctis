#pragma once

#include <string>

#include "Markdown/MarkdownDocument.h"

namespace noctis::md {

// Envoltorio sobre MD4C. Traduce el parsing basado en callbacks de MD4C a
// un Document en memoria simple, consumido por Preview y Export.
class MarkdownParser {
public:
    Document parse(const std::string& markdownSource) const;
};

} // namespace noctis::md
