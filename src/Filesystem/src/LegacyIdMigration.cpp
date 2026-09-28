#include "Filesystem/LegacyIdMigration.h"

#include <sstream>
#include <string>
#include <vector>

namespace noctis::fs {

namespace {

constexpr const char* kMigratedKeys[] = {"favorites", "recents"};

// Los ids heredados son rutas y siempre terminan en .md; los UUID nunca.
bool isLegacyId(const std::string& id) {
    return id.size() > 3 && id.compare(id.size() - 3, 3, ".md") == 0;
}

std::vector<std::string> parse(const std::string& csv) {
    std::vector<std::string> ids;
    std::stringstream stream(csv);
    std::string item;
    while (std::getline(stream, item, ',')) {
        if (!item.empty()) ids.push_back(item);
    }
    return ids;
}

std::string join(const std::vector<std::string>& ids) {
    std::string csv;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) csv += ',';
        csv += ids[i];
    }
    return csv;
}

} // namespace

LegacyIdMigration::LegacyIdMigration(const FilesystemNoteRepository& repository,
                                      core::ISettingsStore& settings)
    : repository_(repository), settings_(settings) {}

void LegacyIdMigration::run() {
    for (const char* key : kMigratedKeys) {
        migrateKey(key);
    }
}

void LegacyIdMigration::migrateKey(const char* key) {
    auto stored = settings_.getString(key);
    if (!stored) return;

    std::vector<std::string> ids = parse(*stored);
    std::vector<std::string> migrated;
    migrated.reserve(ids.size());
    bool changed = false;

    for (const std::string& id : ids) {
        if (!isLegacyId(id)) {
            migrated.push_back(id);
            continue;
        }

        changed = true;
        // Una nota borrada o movida fuera de Noctis ya no se puede traducir:
        // se descarta la entrada en vez de dejar un id que nunca resolverá.
        if (auto current = repository_.idForLegacyPath(id)) {
            migrated.push_back(*current);
        }
    }

    if (changed) {
        settings_.setString(key, join(migrated));
    }
}

} // namespace noctis::fs
