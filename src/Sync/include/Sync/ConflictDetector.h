#pragma once

#include <string>

#include "Core/Models/Note.h"
#include "Core/Ports/ISettingsStore.h"

namespace noctis::sync {

// Detecta si un archivo cambió en disco sin pasar por Noctis (p. ej. otro
// dispositivo lo sincronizó con Syncthing) mientras había una versión sin
// guardar en el editor. No resuelve el conflicto: solo lo señala.
class ConflictDetector {
public:
    explicit ConflictDetector(core::ISettingsStore& settings);

    bool hasExternalChange(const core::NoteId& id, const std::string& diskContent) const;
    void acknowledge(const core::NoteId& id, const std::string& content);

private:
    static std::string checksumOf(const std::string& content);
    std::string keyFor(const core::NoteId& id) const;

    core::ISettingsStore& settings_;
};

} // namespace noctis::sync
