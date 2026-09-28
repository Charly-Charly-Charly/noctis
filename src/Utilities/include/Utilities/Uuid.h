#pragma once

#include <string>

namespace noctis::uuid {

// Genera un UUID v4 en su forma canónica (8-4-4-4-12, minúsculas).
// Es el identificador estable de una nota: vive en su frontmatter y sobrevive
// a renombres y movimientos, incluso a los hechos fuera de Noctis.
std::string generate();

} // namespace noctis::uuid
