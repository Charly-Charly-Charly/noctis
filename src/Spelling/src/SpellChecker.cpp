#include "Spelling/SpellChecker.h"

#include <hunspell.hxx>

namespace noctis::spelling {

SpellChecker::SpellChecker(const std::filesystem::path& affPath,
                            const std::filesystem::path& dicPath) {
    if (std::filesystem::exists(affPath) && std::filesystem::exists(dicPath)) {
        hunspell_ = std::make_unique<Hunspell>(affPath.string().c_str(), dicPath.string().c_str());
    }
}

SpellChecker::~SpellChecker() = default;

bool SpellChecker::isAvailable() const {
    return hunspell_ != nullptr;
}

bool SpellChecker::isCorrect(const std::string& word) const {
    return !hunspell_ || hunspell_->spell(word);
}

} // namespace noctis::spelling
