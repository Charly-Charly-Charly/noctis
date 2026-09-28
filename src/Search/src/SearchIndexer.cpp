#include "Search/SearchIndexer.h"

namespace noctis::search {

SearchIndexer::SearchIndexer(core::ISyncWatcher& watcher, core::INoteRepository& repository,
                              core::ISearchIndex& index)
    : watcher_(watcher), repository_(repository), index_(index) {}

void SearchIndexer::start() {
    watcher_.start([this](const core::NoteId& id, core::ExternalChangeKind kind) {
        switch (kind) {
            case core::ExternalChangeKind::Created:
            case core::ExternalChangeKind::Modified:
                if (auto note = repository_.find(id)) {
                    index_.reindex(*note, repository_.readContent(id));
                }
                break;
            case core::ExternalChangeKind::Removed:
                index_.remove(id);
                break;
            case core::ExternalChangeKind::Conflict:
                break; // lo resuelve Sync/; aquí no se reindexa hasta que se resuelva
        }
    });
}

void SearchIndexer::stop() {
    watcher_.stop();
}

} // namespace noctis::search
