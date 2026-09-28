#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Core/Models/Note.h"

namespace noctis::core {

// Todo lo que cruza esta frontera son tipos primitivos: el Core no sabe que
// del otro lado hay HTTP, JSON o WinHTTP. Eso vive en la implementación
// (Sync/HttpSyncClient), igual que INoteRepository no sabe que hay archivos.

using SyncRevision = std::uint64_t;

struct SyncSession {
    std::string token;
    std::string deviceId;
    std::string userId;
    std::string email;
};

struct RemoteNote {
    NoteId id;
    std::string path;  // ruta relativa tal como la reportó el servidor
    std::string title;
    std::string content;
    SyncRevision rev = 0;
    bool deleted = false;
};

struct RemoteAnnotation {
    std::string id;
    NoteId noteId;
    std::string body;
    std::string quote;
    std::string prefix;
    std::string suffix;
    std::optional<int> offsetHint;
    std::string contentChecksum;
    std::optional<std::string> color;
    SyncRevision rev = 0;
    bool deleted = false;
};

struct PullResult {
    std::vector<RemoteNote> notes;
    std::vector<RemoteAnnotation> annotations;
    SyncRevision rev = 0;
    bool hasMore = false;
};

// Un cambio local que se quiere subir. baseRev es la revisión del servidor
// sobre la que se editó: si el servidor ya avanzó más allá de eso, es un
// conflicto y el push no sobrescribe.
struct NoteChange {
    NoteId id;
    std::string path;
    std::string title;
    std::string content;
    SyncRevision baseRev = 0;
    bool deleted = false;
};

struct AppliedChange {
    NoteId id;
    SyncRevision rev = 0;
};

struct PushConflict {
    NoteId id;
    SyncRevision serverRev = 0;
    SyncRevision baseRev = 0;
};

struct PushResult {
    std::vector<AppliedChange> applied;
    std::vector<PushConflict> conflicts;
    SyncRevision rev = 0;
};

// Cualquier fallo de red, autenticación o del servidor llega como esta
// excepción. SyncService la deja pasar tal cual: quien la muestre (la UI)
// decide cómo traducirla, el Core no depende de ningún framework de UI.
class SyncError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Puerto implementado por Sync/HttpSyncClient. No conoce el disco ni
// INoteRepository: solo habla el protocolo del servidor en términos de datos.
class ISyncClient {
public:
    virtual ~ISyncClient() = default;

    virtual SyncSession login(const std::string& email, const std::string& password,
                               const std::string& deviceName) = 0;

    // Las llamadas siguientes requieren haber llamado antes a setAuthToken().
    virtual void setAuthToken(const std::string& token) = 0;

    virtual PullResult pull(SyncRevision since) = 0;
    virtual PushResult push(const std::vector<NoteChange>& changes) = 0;
};

} // namespace noctis::core
