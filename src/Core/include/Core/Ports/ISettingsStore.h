#pragma once

#include <optional>
#include <string>

namespace noctis::core {

// Puerto implementado por Settings/ (JSON en disco). Solo tipos primitivos
// cruzan esta frontera para mantener Core desacoplado del formato de config.
class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;

    virtual std::optional<std::string> getString(const std::string& key) const = 0;
    virtual void setString(const std::string& key, const std::string& value) = 0;

    virtual std::optional<bool> getBool(const std::string& key) const = 0;
    virtual void setBool(const std::string& key, bool value) = 0;

    virtual std::optional<int> getInt(const std::string& key) const = 0;
    virtual void setInt(const std::string& key, int value) = 0;
};

} // namespace noctis::core
