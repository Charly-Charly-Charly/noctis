#pragma once

#include <string>
#include <vector>

#include "Core/Models/Link.h"
#include "Core/Ports/INoteRepository.h"

namespace noctis::core {

// Wikilinks [[Titulo]]: extracción desde el texto y resolución/creación
// automática de la nota destino cuando todavía no existe.
class LinkService {
public:
    explicit LinkService(INoteRepository& repository);

    std::vector<Link> extractLinks(const NoteId& sourceId, const std::string& content) const;
    Note resolveOrCreate(const Folder& defaultFolder, const std::string& targetTitle);

private:
    INoteRepository& repository_;
};

} // namespace noctis::core
