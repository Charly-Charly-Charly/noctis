#include "Core/Services/SyncService.h"

#include <algorithm>
#include <charconv>
#include <functional>

namespace noctis::core {

namespace {
constexpr const char* kTokenKey = "sync.token";
constexpr const char* kDeviceIdKey = "sync.deviceId";
constexpr const char* kEmailKey = "sync.email";
constexpr const char* kLastPulledRevKey = "sync.lastPulledRev";
constexpr const char* kLastSyncedAtKey = "sync.lastSyncedAt"; // epoch en segundos, como string
// Las notas borradas en el servidor se mueven aquí en vez de eliminarse:
// local-first significa que Noctis nunca destruye trabajo del usuario por su
// cuenta, ni siquiera cuando el borrado viene confirmado del servidor.
constexpr const char* kTrashFolderName = "Eliminadas";

std::string revKeyFor(const NoteId& id) {
    return "sync.rev." + id;
}

std::string hashKeyFor(const NoteId& id) {
    return "sync.hash." + id;
}

SyncRevision parseRevision(const std::string& text) {
    SyncRevision value = 0;
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}
} // namespace

SyncService::SyncService(ISyncClient& client, INoteRepository& repository,
                          ISettingsStore& settings, Folder rootFolder)
    : client_(client), repository_(repository), settings_(settings),
      rootFolder_(std::move(rootFolder)) {
    // Restaura la sesión guardada en el arranque anterior: el usuario no
    // debería tener que iniciar sesión cada vez que abre Noctis.
    if (auto token = settings_.getString(kTokenKey); token && !token->empty()) {
        client_.setAuthToken(*token);
    }
}

bool SyncService::isLoggedIn() const {
    auto token = settings_.getString(kTokenKey);
    return token.has_value() && !token->empty();
}

void SyncService::login(const std::string& email, const std::string& password,
                         const std::string& deviceName) {
    SyncSession session = client_.login(email, password, deviceName);
    client_.setAuthToken(session.token);
    settings_.setString(kTokenKey, session.token);
    settings_.setString(kDeviceIdKey, session.deviceId);
    settings_.setString(kEmailKey, session.email);
}

void SyncService::logout() {
    settings_.setString(kTokenKey, "");
}

SyncSummary SyncService::syncNow() {
    if (!isLoggedIn()) {
        throw SyncError("No has iniciado sesión");
    }

    SyncSummary summary;

    pullLoop(summary);

    std::vector<NoteChange> dirty = collectDirtyChanges();
    if (!dirty.empty()) {
        PushResult result = client_.push(dirty);
        applyPushResult(result, dirty, summary);

        // El servidor ya tiene la versión autoritativa de lo que entró en
        // conflicto; un pull más la trae y sobreescribe el archivo original
        // (la edición local ya quedó a salvo en su copia de conflicto).
        if (!result.conflicts.empty()) {
            pullLoop(summary);
        }
    }

    // Epoch en segundos: suficiente para "hace X minutos/horas" en la UI, sin
    // arrastrar zonas horarias ni formatos de fecha hasta el Core.
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    settings_.setString(kLastSyncedAtKey, std::to_string(seconds));

    return summary;
}

SyncRevision SyncService::lastPulledRev() const {
    auto stored = settings_.getString(kLastPulledRevKey);
    return stored ? parseRevision(*stored) : 0;
}

void SyncService::pullLoop(SyncSummary& summary) {
    while (true) {
        PullResult result = client_.pull(lastPulledRev());

        for (const RemoteNote& note : result.notes) {
            applyPulledNote(note, summary);
        }

        // TODO: aplicar result.annotations una vez exista dónde mostrarlas en
        // el escritorio. Por ahora el servidor ya las guarda; solo falta que
        // el cliente las lea.

        settings_.setString(kLastPulledRevKey, std::to_string(result.rev));

        if (!result.hasMore) break;
    }
}

void SyncService::applyPulledNote(const RemoteNote& note, SyncSummary& summary) {
    auto local = repository_.find(note.id);

    if (note.deleted) {
        if (local) {
            Folder trash = repository_.createFolder(rootFolder_, kTrashFolderName);
            repository_.move(note.id, trash);
        }
        storeSyncState(note.id, note.rev, "");
        ++summary.pulled;
        return;
    }

    if (!local) {
        Folder folder = folderFromRemotePath(note.path);
        repository_.importRemote(note.id, folder, note.title, note.content);
        storeSyncState(note.id, note.rev, contentHash(note.content));
        ++summary.pulled;
        return;
    }

    std::string localContent = repository_.readContent(note.id);
    std::string localHash = contentHash(localContent);
    auto lastSyncedHash = settings_.getString(hashKeyFor(note.id));

    // Un hash vacío es el centinela que deja applyPushResult tras resolver un
    // conflicto en el push: "confía en el servidor, la copia local ya se
    // guardó". Sin distinguirlo, este mismo pull de reconciliación volvería a
    // detectar la nota como dirty y duplicaría la copia de conflicto.
    bool hasKnownBaseline = lastSyncedHash.has_value() && !lastSyncedHash->empty();
    bool localIsDirty = hasKnownBaseline && *lastSyncedHash != localHash;

    if (localIsDirty && localHash != contentHash(note.content)) {
        // Editaste esta nota sin sincronizar, y en el servidor cambió por
        // otro lado. No hay forma automática de saber cuál edición "gana":
        // se preserva la tuya como copia y el archivo original pasa a tener
        // la versión del servidor.
        saveConflictCopy(*local, localContent);
        ++summary.conflicts;
    }

    repository_.writeContent(note.id, note.content);
    storeSyncState(note.id, note.rev, contentHash(note.content));
    ++summary.pulled;
}

void SyncService::saveConflictCopy(const Note& original, const std::string& localContent) {
    Folder folder{original.path.parent_path().filename().string(), original.path.parent_path()};

    std::string title = original.title + " (conflicto)";
    int suffix = 1;
    while (repository_.existsInFolder(folder, title)) {
        title = original.title + " (conflicto " + std::to_string(++suffix) + ")";
    }

    Note copy = repository_.create(folder, title);
    repository_.writeContent(copy.id, localContent);
}

std::vector<NoteChange> SyncService::collectDirtyChanges() {
    std::vector<NoteChange> changes;
    collectFromFolder(rootFolder_, changes);
    return changes;
}

void SyncService::collectFromFolder(const Folder& folder, std::vector<NoteChange>& out) {
    if (folder.path.filename() == kTrashFolderName) return; // nunca se re-sube lo borrado

    for (const Note& note : repository_.listInFolder(folder)) {
        std::string content = repository_.readContent(note.id);
        std::string hash = contentHash(content);

        auto lastSyncedHash = settings_.getString(hashKeyFor(note.id));
        if (lastSyncedHash && *lastSyncedHash == hash) continue; // sin cambios desde el último sync

        NoteChange change;
        change.id = note.id;
        change.path = relativePathOf(note);
        change.title = note.title;
        change.content = content;
        change.baseRev = storedRev(note.id);
        change.deleted = false;
        out.push_back(std::move(change));
    }

    for (const Folder& subfolder : repository_.listSubfolders(folder)) {
        collectFromFolder(subfolder, out);
    }
}

void SyncService::applyPushResult(const PushResult& result, const std::vector<NoteChange>& sent,
                                   SyncSummary& summary) {
    auto findSent = [&sent](const NoteId& id) {
        return std::find_if(sent.begin(), sent.end(),
                             [&id](const NoteChange& change) { return change.id == id; });
    };

    for (const AppliedChange& applied : result.applied) {
        auto it = findSent(applied.id);
        std::string hash = (it != sent.end()) ? contentHash(it->content) : "";
        storeSyncState(applied.id, applied.rev, hash);
        ++summary.pushed;
    }

    for (const PushConflict& conflict : result.conflicts) {
        auto it = findSent(conflict.id);
        if (it != sent.end()) {
            if (auto local = repository_.find(conflict.id)) {
                saveConflictCopy(*local, it->content);
            }
        }

        // Se deja de insistir con esta versión: el pull que sigue a
        // continuación trae la del servidor y la escribe en el archivo
        // original, dejando todo consistente.
        storeSyncState(conflict.id, conflict.serverRev, "");
        ++summary.conflicts;
    }
}

std::string SyncService::relativePathOf(const Note& note) const {
    return std::filesystem::relative(note.path, rootFolder_.path).generic_string();
}

Folder SyncService::folderFromRemotePath(const std::string& relativePath) const {
    std::filesystem::path relative(relativePath);
    std::filesystem::path parent = relative.parent_path();
    std::filesystem::path absolute = rootFolder_.path / parent;

    std::string name = parent.empty() ? rootFolder_.name : parent.filename().string();
    return Folder{name, absolute};
}

std::string SyncService::contentHash(const std::string& content) {
    return std::to_string(std::hash<std::string>{}(content));
}

SyncRevision SyncService::storedRev(const NoteId& id) const {
    auto stored = settings_.getString(revKeyFor(id));
    return stored ? parseRevision(*stored) : 0;
}

void SyncService::storeSyncState(const NoteId& id, SyncRevision rev, const std::string& hash) {
    settings_.setString(revKeyFor(id), std::to_string(rev));
    settings_.setString(hashKeyFor(id), hash);
}

} // namespace noctis::core
