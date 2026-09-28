#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Filesystem/FilesystemNoteRepository.h"
#include "Filesystem/Frontmatter.h"

using namespace noctis::core;
using namespace noctis::fs;

namespace {

// Carpeta de notas temporal y aislada por test.
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
        name << "noctis_test_" << ++counter << "_"
             << std::chrono::steady_clock::now().time_since_epoch().count();
        return name.str();
    }

    std::filesystem::path path_;
};

std::string readRaw(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

TEST(NoteIdentityTest, CreatedNoteGetsIdInFrontmatter) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Note note = repository.create(root.rootFolder(), "Clase 5");

    EXPECT_FALSE(note.id.empty());
    EXPECT_EQ(frontmatterField(splitDocument(readRaw(note.path)).frontmatter, "id"), note.id);
}

TEST(NoteIdentityTest, EditorNeverSeesFrontmatter) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Note note = repository.create(root.rootFolder(), "Clase 5");

    EXPECT_EQ(repository.readContent(note.id).find("id:"), std::string::npos);
    EXPECT_EQ(repository.readContent(note.id), "# Clase 5\n\n");
}

// El encabezado va de más específico a más general: Título, Subcarpeta,
// Carpeta — no la ruta con "/" que se usaba antes.
TEST(NoteIdentityTest, HeadingListsTitleThenFolderPathFromDeepestToShallowest) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Folder universidad = repository.createFolder(root.rootFolder(), "Universidad");
    Folder derechoCivil = repository.createFolder(universidad, "Derecho Civil");

    Note note = repository.create(derechoCivil, "Clase 5");

    EXPECT_EQ(repository.readContent(note.id), "# Clase 5, Derecho Civil, Universidad\n\n");
}

TEST(NoteIdentityTest, HeadingWithoutSubfolderListsTitleThenFolder) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Folder universidad = repository.createFolder(root.rootFolder(), "Universidad");
    Note note = repository.create(universidad, "Clase 5");

    EXPECT_EQ(repository.readContent(note.id), "# Clase 5, Universidad\n\n");
}

TEST(NoteIdentityTest, WritingContentPreservesFrontmatter) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Note note = repository.create(root.rootFolder(), "Clase 5");
    repository.writeContent(note.id, "# Otro título\n");

    EXPECT_EQ(frontmatterField(splitDocument(readRaw(note.path)).frontmatter, "id"), note.id);
    EXPECT_EQ(repository.readContent(note.id), "# Otro título\n");
}

TEST(NoteIdentityTest, IdSurvivesRename) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Note note = repository.create(root.rootFolder(), "Clase 5");
    repository.rename(note.id, "Clase 6");

    auto found = repository.find(note.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->title, "Clase 6");
    EXPECT_EQ(found->id, note.id);
}

TEST(NoteIdentityTest, IdSurvivesMoveBetweenFolders) {
    TempNotesRoot root;
    FilesystemNoteRepository repository(root.path());

    Note note = repository.create(root.rootFolder(), "Clase 5");
    Folder destination = repository.createFolder(root.rootFolder(), "Universidad");
    repository.move(note.id, destination);

    auto found = repository.find(note.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, note.id);
    EXPECT_EQ(found->path.parent_path(), destination.path);
}

// El caso que importa para sync: una nota que llegó por Syncthing o que el
// usuario escribió a mano debe adoptarse con un id sin perder su contenido.
TEST(NoteIdentityTest, AdoptsExternallyCreatedNote) {
    TempNotesRoot root;
    const std::filesystem::path external = root.path() / "Externa.md";
    {
        std::ofstream file(external, std::ios::binary);
        file << "# Externa\n\nEscrita fuera de Noctis\n";
    }

    FilesystemNoteRepository repository(root.path());
    std::vector<Note> notes = repository.listInFolder(root.rootFolder());

    ASSERT_EQ(notes.size(), 1u);
    EXPECT_FALSE(notes[0].id.empty());
    EXPECT_EQ(repository.readContent(notes[0].id), "# Externa\n\nEscrita fuera de Noctis\n");
}

TEST(NoteIdentityTest, ReindexKeepsIdsAlreadyOnDisk) {
    TempNotesRoot root;
    NoteId original;
    {
        FilesystemNoteRepository repository(root.path());
        original = repository.create(root.rootFolder(), "Clase 5").id;
    }

    // Una sesión nueva debe reconocer la misma nota por su id, no reasignarlo.
    FilesystemNoteRepository reopened(root.path());
    EXPECT_TRUE(reopened.find(original).has_value());
}
