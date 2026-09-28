#pragma once

#include <vector>

#include "Core/Models/Note.h"
#include "Core/Ports/ISettingsStore.h"

namespace noctis::core {

// Los favoritos se guardan como lista de NoteId en Settings (JSON), nunca
// como un flag dentro del archivo .md: la nota en sí no cambia.
class FavoritesService {
public:
    explicit FavoritesService(ISettingsStore& settings);

    void toggleFavorite(const NoteId& id);
    bool isFavorite(const NoteId& id) const;
    std::vector<NoteId> listFavorites() const;

private:
    static constexpr const char* kSettingsKey = "favorites";

    ISettingsStore& settings_;
};

} // namespace noctis::core
