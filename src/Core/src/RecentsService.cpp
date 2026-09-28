#include "Core/Services/RecentsService.h"

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

RecentsService::RecentsService(ISettingsStore& settings, std::size_t maxEntries)
    : settings_(settings), maxEntries_(maxEntries) {}

void RecentsService::recordOpened(const NoteId& id) {
    std::vector<NoteId> ids = listRecents();

    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    ids.insert(ids.begin(), id);

    if (ids.size() > maxEntries_) {
        ids.resize(maxEntries_);
    }

    settings_.setString(kSettingsKey, join(ids));
}

std::vector<NoteId> RecentsService::listRecents() const {
    if (auto csv = settings_.getString(kSettingsKey)) {
        return parse(*csv);
    }
    return {};
}

} // namespace noctis::core
