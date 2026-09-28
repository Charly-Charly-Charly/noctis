#include "Core/Services/FavoritesService.h"

#include <algorithm>
#include <sstream>

namespace noctis::core {

namespace {
std::vector<NoteId> parse(const std::string& csv) {
    std::vector<NoteId> ids;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) ids.push_back(item);
    }
    return ids;
}

std::string join(const std::vector<NoteId>& ids) {
    std::string csv;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) csv += ',';
        csv += ids[i];
    }
    return csv;
}
} // namespace

FavoritesService::FavoritesService(ISettingsStore& settings) : settings_(settings) {}

void FavoritesService::toggleFavorite(const NoteId& id) {
    std::vector<NoteId> ids = listFavorites();
    auto it = std::find(ids.begin(), ids.end(), id);

    if (it != ids.end()) {
        ids.erase(it);
    } else {
        ids.push_back(id);
    }

    settings_.setString(kSettingsKey, join(ids));
}

bool FavoritesService::isFavorite(const NoteId& id) const {
    std::vector<NoteId> ids = listFavorites();
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

std::vector<NoteId> FavoritesService::listFavorites() const {
    if (auto csv = settings_.getString(kSettingsKey)) {
        return parse(*csv);
    }
    return {};
}

} // namespace noctis::core
