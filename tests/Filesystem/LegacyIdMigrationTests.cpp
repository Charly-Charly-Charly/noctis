#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "Filesystem/FilesystemNoteRepository.h"
#include "Filesystem/LegacyIdMigration.h"

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
        name << "noctis_migration_" << ++counter << "_"
             << std::chrono::steady_clock::now().time_since_epoch().count();
        return name.str();
    }

    std::filesystem::path path_;
};

} // namespace

TEST(LegacyIdMigrationTest, TranslatesPathIdToCurrentUuid) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    Note note = repository.create(root.rootFolder(), "Clase 5");

    FakeSettingsStore settings;
    settings.setString("recents", "Clase 5.md");

    LegacyIdMigration(repository, settings).run();

    EXPECT_EQ(settings.getString("recents"), note.id);
}

TEST(LegacyIdMigrationTest, DropsEntriesThatNoLongerResolve) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    Note note = repository.create(root.rootFolder(), "Clase 5");

    FakeSettingsStore settings;
    settings.setString("favorites", "Borrada.md,Clase 5.md");

    LegacyIdMigration(repository, settings).run();

    EXPECT_EQ(settings.getString("favorites"), note.id);
}

TEST(LegacyIdMigrationTest, LeavesUuidsUntouched) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    Note note = repository.create(root.rootFolder(), "Clase 5");

    FakeSettingsStore settings;
    settings.setString("recents", note.id);

    LegacyIdMigration(repository, settings).run();

    EXPECT_EQ(settings.getString("recents"), note.id);
}

TEST(LegacyIdMigrationTest, IsIdempotent) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());
    Note note = repository.create(root.rootFolder(), "Clase 5");

    FakeSettingsStore settings;
    settings.setString("recents", "Clase 5.md");

    LegacyIdMigration migration(repository, settings);
    migration.run();
    migration.run();

    EXPECT_EQ(settings.getString("recents"), note.id);
}
