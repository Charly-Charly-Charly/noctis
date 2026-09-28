#pragma once

#include <filesystem>
#include <memory>
#include <string>

class Hunspell;

namespace noctis::spelling {

// Envoltorio delgado sobre Hunspell: valida palabras sueltas contra el
// diccionario cargado. Si los archivos .aff/.dic no existen (instalación
// incompleta, o el diccionario no se pudo descargar durante el build),
// isAvailable() devuelve false e isCorrect() no marca nada como incorrecto
// — degradar en silencio es preferible a que el editor deje de abrir notas.
class SpellChecker {
public:
    SpellChecker(const std::filesystem::path& affPath, const std::filesystem::path& dicPath);
    ~SpellChecker();

    bool isAvailable() const;
    bool isCorrect(const std::string& word) const;

private:
    std::unique_ptr<Hunspell> hunspell_;
};

} // namespace noctis::spelling
