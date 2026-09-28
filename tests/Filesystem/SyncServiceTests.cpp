#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <map>
#include <sstream>

#include "Core/Services/SyncService.h"
#include "Filesystem/FilesystemNoteRepository.h"

using namespace noctis::core;
using namespace noctis::fs;

namespace {

class FakeSettingsStore : public ISettingsStore {
public:
    std::optional<std::string> getString(const std::string& key) const override {
        auto it = values_.find(key);
        if (it == values_.end()) return std::nullopt;
        return it->second;
    }

    void setString(const std::string& key, const std::string& value) override {
        values_[key] = value;
    }

    std::optional<bool> getBool(const std::string&) const override { return std::nullopt; }
    void setBool(const std::string&, bool) override {}

    std::optional<int> getInt(const std::string&) const override { return std::nullopt; }
    void setInt(const std::string&, int) override {}

private:
    std::map<std::string, std::string> values_;
};

// Cliente de sync sin red: las respuestas de pull se consumen en orden de una
// cola, y los conflictos de push se disparan por id según se configuren.
class FakeSyncClient : public ISyncClient {
public:
    SyncSession login(const std::string& email, const std::string&, const std::string&) override {
        ++loginCalls;
        return SyncSession{"fake-token", "device-1", "user-1", email};
    }

    void setAuthToken(const std::string& token) override { lastToken = token; }

    PullResult pull(SyncRevision since) override {
        pullCallsSince.push_back(since);
        if (pullResponses.empty()) return PullResult{};
        PullResult next = pullResponses.front();
        pullResponses.erase(pullResponses.begin());
        return next;
    }

    PushResult push(const std::vector<NoteChange>& changes) override {
        pushedBatches.push_back(changes);

        PushResult result;
        for (const NoteChange& change : changes) {
            auto conflict = conflictedIds.find(change.id);
            if (conflict != conflictedIds.end()) {
                result.conflicts.push_back(PushConflict{change.id, conflict->second, change.baseRev});
            } else {
                result.applied.push_back(AppliedChange{change.id, ++nextRev});
            }
        }
        result.rev = nextRev;
        return result;
    }

    int loginCalls = 0;
    std::string lastToken;
    std::vector<SyncRevision> pullCallsSince;
    std::vector<PullResult> pullResponses;
    std::vector<std::vector<NoteChange>> pushedBatches;
    std::map<NoteId, SyncRevision> conflictedIds;
    SyncRevision nextRev = 0;
};

class TempNotesRoot {
public:
    TempNotesRoot() : path_(std::filesystem::temp_directory_path() / uniqueName()) {
        std::filesystem::create_directories(path_);
    }

    ~TempNotesRoot() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    TempNotesRoot(const TempNotesRoot&) = delete;
    TempNotesRoot& operator=(const TempNotesRoot&) = delete;

    const std::filesystem::path& path() const { return path_; }
    Folder rootFolder() const { return Folder{"Notas", path_}; }

private:
    static std::string uniqueName() {
        static int counter = 0;
        std::ostringstream name;
        name << "noctis_sync_" << ++counter << "_"
             << std::chrono::steady_clock::now().time_since_epoch().count();
        return name.str();
    }

    std::filesystem::path path_;
};

std::optional<Note> findByTitleContaining(FilesystemNoteRepository& repository, const Folder& folder,
                                           const std::string& needle) {
    for (const Note& note : repository.listInFolder(folder)) {
        if (note.title.find(needle) != std::string::npos) return note;
    }
    return std::nullopt;
}

} // namespace

TEST(SyncServiceTest, LoginStoresSessionAndMarksLoggedIn) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());

    EXPECT_FALSE(sync.isLoggedIn());

    sync.login("persona@ejemplo.com", "clave-de-prueba-larga", "Laptop");

    EXPECT_TRUE(sync.isLoggedIn());
    EXPECT_EQ(client.lastToken, "fake-token");
    EXPECT_EQ(settings.getString("sync.token"), "fake-token");
}

TEST(SyncServiceTest, SyncNowWithoutLoginThrows) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());

    EXPECT_THROW(sync.syncNow(), SyncError);
}

TEST(SyncServiceTest, PullCreatesNewLocalNote) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    RemoteNote remote;
    remote.id = "11111111-1111-1111-1111-111111111111";
    remote.path = "Clase 5.md";
    remote.title = "Clase 5";
    remote.content = "# Clase 5\nContenido remoto";
    remote.rev = 1;

    PullResult result;
    result.notes = {remote};
    result.rev = 1;
    client.pullResponses = {result};

    SyncSummary summary = sync.syncNow();

    EXPECT_EQ(summary.pulled, 1);
    EXPECT_EQ(summary.conflicts, 0);

    auto note = repository.find(remote.id);
    ASSERT_TRUE(note.has_value());
    EXPECT_EQ(repository.readContent(remote.id), remote.content);
}

TEST(SyncServiceTest, PullUpdatesUnmodifiedLocalNoteWithoutConflict) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    RemoteNote remote;
    remote.id = "22222222-2222-2222-2222-222222222222";
    remote.path = "Clase 5.md";
    remote.title = "Clase 5";
    remote.content = "# Clase 5\nVersión 1";
    remote.rev = 1;

    PullResult first;
    first.notes = {remote};
    first.rev = 1;
    client.pullResponses = {first};
    sync.syncNow();

    RemoteNote updated = remote;
    updated.content = "# Clase 5\nVersión 2, editada en otro dispositivo";
    updated.rev = 2;

    PullResult second;
    second.notes = {updated};
    second.rev = 2;
    client.pullResponses = {second};

    SyncSummary summary = sync.syncNow();

    EXPECT_EQ(summary.conflicts, 0);
    EXPECT_EQ(repository.readContent(remote.id), updated.content);
}

TEST(SyncServiceTest, PullConflictPreservesLocalEditAsCopy) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    RemoteNote remote;
    remote.id = "33333333-3333-3333-3333-333333333333";
    remote.path = "Clase 5.md";
    remote.title = "Clase 5";
    remote.content = "# Clase 5\nVersión original";
    remote.rev = 1;

    PullResult first;
    first.notes = {remote};
    first.rev = 1;
    client.pullResponses = {first};
    sync.syncNow();

    // Edición local sin sincronizar.
    repository.writeContent(remote.id, "# Clase 5\nEdición local sin sincronizar");

    // El servidor, mientras tanto, recibió otra edición.
    RemoteNote updated = remote;
    updated.content = "# Clase 5\nEdición desde el teléfono";
    updated.rev = 2;

    PullResult second;
    second.notes = {updated};
    second.rev = 2;
    client.pullResponses = {second};

    SyncSummary summary = sync.syncNow();

    EXPECT_GE(summary.conflicts, 1);
    // El archivo original queda con la verdad del servidor...
    EXPECT_EQ(repository.readContent(remote.id), updated.content);

    // ...y la edición local no se perdió: vive en una copia de conflicto.
    auto copy = findByTitleContaining(repository, root.rootFolder(), "(conflicto)");
    ASSERT_TRUE(copy.has_value());
    EXPECT_EQ(repository.readContent(copy->id), "# Clase 5\nEdición local sin sincronizar");
}

TEST(SyncServiceTest, PushSyncsNewLocalNoteThenStopsRepushingUnchanged) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    Note note = repository.create(root.rootFolder(), "Nota nueva");
    repository.writeContent(note.id, "Contenido local");

    client.pullResponses = {PullResult{}};
    SyncSummary first = sync.syncNow();

    EXPECT_EQ(first.pushed, 1);
    ASSERT_EQ(client.pushedBatches.size(), 1u);
    ASSERT_EQ(client.pushedBatches[0].size(), 1u);
    EXPECT_EQ(client.pushedBatches[0][0].content, "Contenido local");
    EXPECT_EQ(client.pushedBatches[0][0].baseRev, 0u);

    // Nada cambió localmente: la segunda sincronización no debe re-subirla.
    client.pullResponses = {PullResult{}};
    SyncSummary second = sync.syncNow();

    EXPECT_EQ(second.pushed, 0);
    EXPECT_EQ(client.pushedBatches.size(), 1u);
}

TEST(SyncServiceTest, PushConflictPreservesLocalEditAndReconcilesWithServer) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    Note note = repository.create(root.rootFolder(), "Compartida");
    repository.writeContent(note.id, "Versión local");

    // El servidor rechaza el push por conflicto...
    client.conflictedIds[note.id] = 5;

    // ...y el pull de reconciliación que sigue entrega la versión autoritativa.
    RemoteNote serverVersion;
    serverVersion.id = note.id;
    serverVersion.path = "Compartida.md";
    serverVersion.title = "Compartida";
    serverVersion.content = "Versión del servidor";
    serverVersion.rev = 5;

    PullResult initialPull; // primer pull de syncNow(): vacío
    PullResult reconcilePull;
    reconcilePull.notes = {serverVersion};
    reconcilePull.rev = 5;
    client.pullResponses = {initialPull, reconcilePull};

    SyncSummary summary = sync.syncNow();

    EXPECT_EQ(summary.conflicts, 1);
    EXPECT_EQ(repository.readContent(note.id), "Versión del servidor");

    auto copy = findByTitleContaining(repository, root.rootFolder(), "(conflicto)");
    ASSERT_TRUE(copy.has_value());
    EXPECT_NE(copy->id, note.id);
    EXPECT_EQ(repository.readContent(copy->id), "Versión local");
}

TEST(SyncServiceTest, PullLoopFollowsHasMoreAcrossMultiplePages) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    RemoteNote first;
    first.id = "44444444-4444-4444-4444-444444444444";
    first.path = "Uno.md";
    first.title = "Uno";
    first.content = "Contenido uno";
    first.rev = 1;

    RemoteNote second;
    second.id = "55555555-5555-5555-5555-555555555555";
    second.path = "Dos.md";
    second.title = "Dos";
    second.content = "Contenido dos";
    second.rev = 2;

    PullResult page1;
    page1.notes = {first};
    page1.rev = 1;
    page1.hasMore = true; // el cliente debe seguir pidiendo

    PullResult page2;
    page2.notes = {second};
    page2.rev = 2;
    page2.hasMore = false;

    client.pullResponses = {page1, page2};

    SyncSummary summary = sync.syncNow();

    EXPECT_EQ(summary.pulled, 2);
    ASSERT_EQ(client.pullCallsSince.size(), 2u);
    EXPECT_EQ(client.pullCallsSince[0], 0u); // primera página: desde el principio
    EXPECT_EQ(client.pullCallsSince[1], 1u); // segunda página: retoma donde llegó la primera
    EXPECT_TRUE(repository.find(first.id).has_value());
    EXPECT_TRUE(repository.find(second.id).has_value());
}

TEST(SyncServiceTest, DeletedRemoteNoteMovesLocalFileToTrashInsteadOfDestroyingIt) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    FakeSettingsStore settings;
    FakeSyncClient client;
    SyncService sync(client, repository, settings, root.rootFolder());
    sync.login("a@b.com", "clave-de-prueba-larga", "Laptop");

    RemoteNote remote;
    remote.id = "66666666-6666-6666-6666-666666666666";
    remote.path = "Clase 5.md";
    remote.title = "Clase 5";
    remote.content = "# Clase 5";
    remote.rev = 1;

    PullResult first;
    first.notes = {remote};
    first.rev = 1;
    client.pullResponses = {first};
    sync.syncNow();

    RemoteNote deleted = remote;
    deleted.deleted = true;
    deleted.rev = 2;

    PullResult second;
    second.notes = {deleted};
    second.rev = 2;
    client.pullResponses = {second};

    SyncSummary summary = sync.syncNow();

    EXPECT_EQ(summary.pulled, 1);

    auto note = repository.find(remote.id);
    ASSERT_TRUE(note.has_value()); // no se destruyó...
    EXPECT_EQ(note->path.parent_path().filename(), "Eliminadas"); // ...se archivó
}
