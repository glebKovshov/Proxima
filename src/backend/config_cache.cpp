#include "proxima/backend/config_cache.hpp"

#include <fstream>

namespace proxima::backend {

ConfigCache::ConfigCache(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<std::string> ConfigCache::read() const {
    std::ifstream stream(path_, std::ios::binary);
    if (!stream) return std::nullopt;
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool ConfigCache::write(const std::string& payload) const {
    std::error_code error;
    if (!path_.parent_path().empty()) {
        std::filesystem::create_directories(path_.parent_path(), error);
        if (error) return false;
    }
    const auto temporary = path_.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) return false;
        stream.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        if (!stream.good()) return false;
    }
    (void)std::filesystem::remove(path_, error);
    error.clear();
    std::filesystem::rename(temporary, path_, error);
    return !error;
}

const std::filesystem::path& ConfigCache::path() const noexcept { return path_; }

ConfigResolver::ConfigResolver(ConfigCache cache) : cache_(std::move(cache)) {}

ConfigLoadResult ConfigResolver::resolve(const std::optional<std::string>& remotePayload) const {
    if (remotePayload) {
        std::string error;
        if (const auto config = ConfigManager::loadValidated(*remotePayload, &error)) {
            (void)cache_.write(*remotePayload);
            return {*config, ConfigSource::Remote, {}};
        }
    }

    if (const auto cached = cache_.read()) {
        std::string error;
        if (const auto config = ConfigManager::loadValidated(*cached, &error)) {
            return {*config, ConfigSource::Cache, "Backend unavailable; using cached configuration."};
        }
    }
    return {ConfigManager::defaultConfig(), ConfigSource::BuiltIn,
            "Backend and cache unavailable; using built-in configuration."};
}

} // namespace proxima::backend
