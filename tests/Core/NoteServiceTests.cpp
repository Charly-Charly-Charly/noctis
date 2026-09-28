#include <gtest/gtest.h>

#include <algorithm>
#include <map>

#include "Core/Services/NoteService.h"

using namespace noctis::core;

namespace {

class FakeNoteRepository : public INoteRepository {
public:
    std::optional<Note> find(const NoteId& id) const override {
        auto it = notes_.find(id);
        if (it == notes_.end()) return std::nullopt;
        return it->second;
    }

    std::vector<Note> listInFolder(const Folder& folder) const override {
        std::vector<Note> result;
        for (const auto& [id, note] : notes_) {
            if (note.path.parent_path() == folder.path) result.push_back(note);
        }
        return result;
    }

    std::string readContent(const NoteId& id) const override { return contents_.at(id); }

    void writeContent(const NoteId& id, const std::string& content) override {
        contents_[id] = content;
    }

    Note create(const Folder& folder, const std::string& title) override {
        Note note;
        note.id = title;
        note.title = title;
        note.path = folder.path / (title + ".md");
        notes_[note.id] = note;
        contents_[note.id] = "";
        return note;
    }

    Note importRemote(const NoteId& id, const Folder& folder, const std::string& title,
                       const std::string& content) override {
        Note note;
        note.id = id;
        note.title = title;
        note.path = folder.path / (title + ".md");
        notes_[note.id] = note;
        contents_[note.id] = content;
        return note;
    }

    void rename(const NoteId&, const std::string&) override {}
    void move(const NoteId&, const Folder&) override {}
    void remove(const NoteId& id) override { notes_.erase(id); }

    bool existsInFolder(const Folder&, const std::string& title) const override {
        return notes_.contains(title);
    }

    std::vector<Folder> listSubfolders(const Folder&) const override { return {}; }
    Folder createFolder(const Folder&, const std::string&) override { return {}; }
    void renameFolder(const Folder&, const std::string&) override {}
    void moveFolder(const Folder&, const Folder&) override {}
    void removeFolder(const Folder&) override {}

private:
    std::map<NoteId, Note> notes_;
    std::map<NoteId, std::string> contents_;
};

class FakeSearchIndex : public ISearchIndex {
public:
    std::vector<SearchResult> search(const std::string&) const override { return {}; }
    void reindex(const Note& note, const std::string&) override { indexed.push_back(note.id); }
    void remove(const NoteId& id) override {
        indexed.erase(std::remove(indexed.begin(), indexed.end(), id), indexed.end());
    }

    std::vector<NoteId> indexed;
};

} // namespace

TEST(NoteServiceTest, CreateNoteIndexesIt) {
    FakeNoteRepository repository;
    FakeSearchIndex index;
    NoteService service(repository, index);

    Note note = service.createNote(Folder{"Universidad", "Universidad"}, "Derechos Humanos");

    EXPECT_EQ(note.title, "Derechos Humanos");
    ASSERT_EQ(index.indexed.size(), 1u);
    EXPECT_EQ(index.indexed[0], "Derechos Humanos");
}

TEST(NoteServiceTest, CreateDuplicateThrows) {
    FakeNoteRepository repository;
    FakeSearchIndex index;
    NoteService service(repository, index);

    service.createNote(Folder{"Universidad", "Universidad"}, "Derechos Humanos");

    EXPECT_THROW(service.createNote(Folder{"Universidad", "Universidad"}, "Derechos Humanos"),
                 std::runtime_error);
}

TEST(NoteServiceTest, DeleteNoteRemovesFromIndex) {
    FakeNoteRepository repository;
    FakeSearchIndex index;
    NoteService service(repository, index);

    Note note = service.createNote(Folder{"Universidad", "Universidad"}, "Derechos Humanos");
    service.deleteNote(note.id);

    EXPECT_TRUE(index.indexed.empty());
}
