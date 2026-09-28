#pragma once

#include <vector>

#include "Core/Models/Note.h"
#include "Core/Ports/ISettingsStore.h"

namespace noctis::core {

// Historial de las últimas notas abiertas, más reciente primero. Igual que
// favoritos, vive en Settings, nunca dentro de los .md.
class RecentsService {
public:
    explicit RecentsService(ISettingsStore& settings, std::size_t maxEntries = 20);

    void recordOpened(const NoteId& id);
    std::vector<NoteId> listRecents() const;

private:
    static constexpr const char* kSettingsKey = "recents";

    ISettingsStore& settings_;
    std::size_t maxEntries_;
};

} // namespace noctis::core
