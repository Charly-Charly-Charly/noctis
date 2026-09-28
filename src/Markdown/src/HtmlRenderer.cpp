#include "Markdown/HtmlRenderer.h"

#include <md4c-html.h>

#include "Core/Services/TagService.h"

namespace noctis::md {

namespace {
void appendChunk(const MD_CHAR* text, MD_SIZE size, void* userdata) {
    static_cast<std::string*>(userdata)->append(text, size);
}
} // namespace

std::string HtmlRenderer::render(const std::string& markdownSource) const {
    // Las etiquetas (#tag) son metadata de organización, no contenido para
    // el lector final: se sacan antes de renderizar, tanto para la vista
    // previa como para exportar a HTML/PDF. El .md real en disco nunca pasa
    // por acá, así que conserva las etiquetas intactas.
    std::string withoutTags = core::TagService{}.stripTags(markdownSource);

    std::string html;
    md_html(withoutTags.c_str(), static_cast<MD_SIZE>(withoutTags.size()), appendChunk, &html,
            MD_DIALECT_GITHUB, 0);
    return html;
}

} // namespace noctis::md
