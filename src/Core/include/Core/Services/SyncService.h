#pragma once

#include <string>
#include <vector>

#include "Core/Models/Folder.h"
#include "Core/Models/Note.h"
#include "Core/Ports/INoteRepository.h"
#include "Core/Ports/ISettingsStore.h"
#include "Core/Ports/ISyncClient.h"

namespace noctis::core {

struct SyncSummary {
    int pulled = 0;
    int pushed = 0;
    int conflicts = 0;
};

// Orquesta la sincronización entre el disco local (INoteRepository) y el
// servidor (ISyncClient). El disco sigue siendo la fuente de verdad para lo
// que el usuario está editando: si una nota cambió en el servidor mientras
// también cambiaba localmente sin sincronizar, la edición local se preserva
// como una copia "(conflicto)" en vez de perderse. Nunca se sobrescribe
// trabajo del usuario en silencio.
class SyncService {
public:
    SyncService(ISyncClient& client, INoteRepository& repository, ISettingsStore& settings,
                Folder rootFolder);

    bool isLoggedIn() const;
    void login(const std::string& email, const std::string& password,
               const std::string& deviceName);
    void logout();

    // Lanza SyncError si no hay sesión iniciada o si falla la red/el servidor.
    SyncSummary syncNow();

private:
    void pullLoop(SyncSummary& summary);
    void applyPulledNote(const RemoteNote& note, SyncSummary& summary);

    std::vector<NoteChange> collectDirtyChanges();
    void collectFromFolder(const Folder& folder, std::vector<NoteChange>& out);
    void applyPushResult(const PushResult& result, const std::vector<NoteChange>& sent,
                          SyncSummary& summary);

    // Preserva una edición local que no se pudo reconciliar automáticamente:
    // crea "<título> (conflicto)" con el contenido local, dejando el archivo
    // original libre para que reciba la versión del servidor.
    void saveConflictCopy(const Note& original, const std::string& localContent);

    std::string relativePathOf(const Note& note) const;
    Folder folderFromRemotePath(const std::string& relativePath) const;

    static std::string contentHash(const std::string& content);
    SyncRevision storedRev(const NoteId& id) const;
    void storeSyncState(const NoteId& id, SyncRevision rev, const std::string& hash);
    SyncRevision lastPulledRev() const;

    ISyncClient& client_;
    INoteRepository& repository_;
    ISettingsStore& settings_;
    Folder rootFolder_;
};

} // namespace noctis::core
