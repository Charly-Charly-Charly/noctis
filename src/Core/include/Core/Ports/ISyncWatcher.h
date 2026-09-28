#pragma once

#include <functional>

#include "Core/Models/Note.h"

namespace noctis::core {

enum class ExternalChangeKind {
    Created,
    Modified,
    Removed,
    Conflict, // el archivo cambió en disco y en el índice desde el último sync conocido
};

// Puerto implementado por Sync/ (y respaldado por Filesystem/ para la
// observación real de disco). Noctis no sincroniza nada por sí mismo: solo
// observa cambios que Syncthing/Git/Dropbox/etc. ya escribieron en disco.
class ISyncWatcher {
public:
    virtual ~ISyncWatcher() = default;

    using ChangeCallback = std::function<void(const NoteId&, ExternalChangeKind)>;

    virtual void start(ChangeCallback onChange) = 0;
    virtual void stop() = 0;
};

} // namespace noctis::core
