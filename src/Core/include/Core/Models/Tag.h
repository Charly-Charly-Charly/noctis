#pragma once

#include <string>

#include "Core/Models/Note.h"

namespace noctis::core {

struct TagOccurrence {
    std::string tag;
    NoteId noteId;
};

} // namespace noctis::core
