#pragma once

#include <optional>
#include <string>

namespace noctis::fs {

// Un archivo .md partido en su bloque YAML inicial y el cuerpo que el usuario
// realmente edita. El frontmatter guarda metadatos de Noctis (hoy solo el id
// estable); el editor nunca lo muestra, así que el archivo sigue siendo un
// Markdown limpio para cualquier otra herramienta.
struct Document {
    std::string frontmatter; // contenido entre los delimitadores, sin los "---"
    std::string body;
};

// Si el texto no empieza con un bloque "---", todo se considera cuerpo.
Document splitDocument(const std::string& raw);
std::string joinDocument(const Document& document);

std::optional<std::string> frontmatterField(const std::string& frontmatter,
                                             const std::string& key);

// Reemplaza el valor si la clave ya existe; si no, la añade al final.
std::string upsertFrontmatterField(const std::string& frontmatter, const std::string& key,
                                    const std::string& value);

} // namespace noctis::fs
