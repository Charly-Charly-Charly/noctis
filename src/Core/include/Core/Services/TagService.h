#pragma once

#include <string>
#include <vector>

#include "Core/Models/Folder.h"
#include "Core/Models/Tag.h"
#include "Core/Ports/INoteRepository.h"

namespace noctis::core {

// Las etiquetas son texto Markdown (#tag) dentro del contenido: no existe
// almacenamiento separado. Este servicio solo sabe extraerlas de un texto.
class TagService {
public:
    std::vector<std::string> extractTags(const std::string& content) const;

    // Devuelve el contenido sin los tokens "#tag": para renderizar (vista
    // previa, exportar a HTML/PDF), donde la etiqueta es metadata de
    // organización y no algo que el lector final necesite ver. Nunca se usa
    // para lo que se guarda en disco — el .md real conserva las etiquetas
    // tal cual las escribió el usuario.
    std::string stripTags(const std::string& content) const;

    // Recorre todo el árbol agregando una TagOccurrence por cada etiqueta de
    // cada nota. Costoso (lee el contenido de cada nota): pensado para
    // diálogos puntuales (ej. el selector de etiquetas), no para refrescos
    // frecuentes.
    std::vector<TagOccurrence> collectAll(INoteRepository& repository, const Folder& root) const;
};

} // namespace noctis::core
