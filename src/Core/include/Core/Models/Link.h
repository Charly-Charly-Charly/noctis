#pragma once

#include <optional>
#include <string>

#include "Core/Models/Note.h"

namespace noctis::core {

// Un wikilink [[Titulo]] encontrado en el contenido de una nota. `targetId`
// queda vacío si la nota destino todavía no existe (enlace pendiente).
struct Link {
    NoteId sourceId;
    std::string targetTitle;
    std::optional<NoteId> targetId;
};

} // namespace noctis::core
