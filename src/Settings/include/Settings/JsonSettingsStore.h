#pragma once

#include <filesystem>
#include <mutex>

#include <nlohmann/json.hpp>

#include "Core/Ports/ISettingsStore.h"

namespace noctis::settings {

// Config persistida como JSON plano en disco, con "schemaVersion" para
// permitir migraciones futuras sin romper configuraciones antiguas.
class JsonSettingsStore : public core::ISettingsStore {
public:
    explicit JsonSettingsStore(std::filesystem::path filePath);

    std::optional<std::string> getString(const std::string& key) const override;
    void setString(const std::string& key, const std::string& value) override;

    std::optional<bool> getBool(const std::string& key) const override;
    void setBool(const std::string& key, bool value) override;

    std::optional<int> getInt(const std::string& key) const override;
    void setInt(const std::string& key, int value) override;

private:
    void load();
    void save() const;

    std::filesystem::path filePath_;
    nlohmann::json data_;
    mutable std::mutex mutex_;

    static constexpr int kSchemaVersion = 1;
};

} // namespace noctis::settings
