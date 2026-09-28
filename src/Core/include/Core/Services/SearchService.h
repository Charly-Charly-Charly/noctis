#pragma once

#include <string>
#include <vector>

#include "Core/Ports/ISearchIndex.h"

namespace noctis::core {

class SearchService {
public:
    explicit SearchService(ISearchIndex& index);

    std::vector<SearchResult> search(const std::string& query) const;

private:
    ISearchIndex& index_;
};

} // namespace noctis::core
