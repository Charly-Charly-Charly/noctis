#include "Settings/JsonSettingsStore.h"

#include <fstream>

namespace noctis::settings {

JsonSettingsStore::JsonSettingsStore(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {
    load();
}

void JsonSettingsStore::load() {
    std::lock_guard lock(mutex_);

    if (!std::filesystem::exists(filePath_)) {
        data_ = nlohmann::json{{"schemaVersion", kSchemaVersion}};
        return;
    }

    std::ifstream file(filePath_);
    file >> data_;
}

void JsonSettingsStore::save() const {
    std::filesystem::create_directories(filePath_.parent_path());
    std::ofstream file(filePath_);
    file << data_.dump(2);
}

std::optional<std::string> JsonSettingsStore::getString(const std::string& key) const {
    std::lock_guard lock(mutex_);
    if (!data_.contains(key) || !data_[key].is_string()) return std::nullopt;
    return data_[key].get<std::string>();
}

void JsonSettingsStore::setString(const std::string& key, const std::string& value) {
    {
        std::lock_guard lock(mutex_);
        data_[key] = value;
    }
    save();
}

std::optional<bool> JsonSettingsStore::getBool(const std::string& key) const {
    std::lock_guard lock(mutex_);
    if (!data_.contains(key) || !data_[key].is_boolean()) return std::nullopt;
    return data_[key].get<bool>();
}

void JsonSettingsStore::setBool(const std::string& key, bool value) {
    {
        std::lock_guard lock(mutex_);
        data_[key] = value;
    }
    save();
}

std::optional<int> JsonSettingsStore::getInt(const std::string& key) const {
    std::lock_guard lock(mutex_);
    if (!data_.contains(key) || !data_[key].is_number_integer()) return std::nullopt;
    return data_[key].get<int>();
}

void JsonSettingsStore::setInt(const std::string& key, int value) {
    {
        std::lock_guard lock(mutex_);
        data_[key] = value;
    }
    save();
}

} // namespace noctis::settings
