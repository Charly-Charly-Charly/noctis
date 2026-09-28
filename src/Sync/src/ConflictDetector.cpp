#include "Sync/ConflictDetector.h"

#include <functional>

namespace noctis::sync {

ConflictDetector::ConflictDetector(core::ISettingsStore& settings) : settings_(settings) {}

std::string ConflictDetector::checksumOf(const std::string& content) {
    return std::to_string(std::hash<std::string>{}(content));
}

std::string ConflictDetector::keyFor(const core::NoteId& id) const {
    return "sync.checksum." + id;
}

bool ConflictDetector::hasExternalChange(const core::NoteId& id,
                                          const std::string& diskContent) const {
    auto known = settings_.getString(keyFor(id));
    if (!known) return false; // primera vez que se ve esta nota: no hay conflicto posible
    return *known != checksumOf(diskContent);
}

void ConflictDetector::acknowledge(const core::NoteId& id, const std::string& content) {
    settings_.setString(keyFor(id), checksumOf(content));
}

} // namespace noctis::sync
