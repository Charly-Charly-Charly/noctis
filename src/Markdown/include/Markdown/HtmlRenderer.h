#pragma once

#include <string>

namespace noctis::md {

// Envoltorio sobre md4c-html: convierte Markdown a HTML completo. Lo usan
// tanto la vista previa del Editor como Export, para no duplicar la
// integración con MD4C en dos sitios.
class HtmlRenderer {
public:
    std::string render(const std::string& markdownSource) const;
};

} // namespace noctis::md
