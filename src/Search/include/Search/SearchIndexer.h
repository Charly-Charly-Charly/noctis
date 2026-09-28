#pragma once

#include "Core/Ports/INoteRepository.h"
#include "Core/Ports/ISearchIndex.h"
#include "Core/Ports/ISyncWatcher.h"

namespace noctis::search {

// Conecta el watcher de filesystem con el índice: cuando un .md cambia en
// disco (por el propio usuario o por un servicio de sync externo), reindexa
// automáticamente. Es el único lugar donde ISyncWatcher e ISearchIndex se
// conocen entre sí.
class SearchIndexer {
public:
    SearchIndexer(core::ISyncWatcher& watcher, core::INoteRepository& repository,
                  core::ISearchIndex& index);

    void start();
    void stop();

private:
    core::ISyncWatcher& watcher_;
    core::INoteRepository& repository_;
    core::ISearchIndex& index_;
};

} // namespace noctis::search
