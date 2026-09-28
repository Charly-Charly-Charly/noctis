#include "Core/Services/SearchService.h"

namespace noctis::core {

SearchService::SearchService(ISearchIndex& index) : index_(index) {}

std::vector<SearchResult> SearchService::search(const std::string& query) const {
    return index_.search(query);
}

} // namespace noctis::core
