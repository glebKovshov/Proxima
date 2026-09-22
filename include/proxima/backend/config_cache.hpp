#pragma once

#include "proxima/backend/config.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace proxima::backend {

class ConfigCache {
public:
    explicit ConfigCache(std::filesystem::path path);

    [[nodiscard]] std::optional<std::string> read() const;
    [[nodiscard]] bool write(const std::string& payload) const;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
    std::filesystem::path path_;
};

enum class ConfigSource { Remote, Cache, BuiltIn };

struct ConfigLoadResult {
    BackendConfig config;
    ConfigSource source{ConfigSource::BuiltIn};
    std::string warning;
};

class ConfigResolver {
public:
    explicit ConfigResolver(ConfigCache cache);

    [[nodiscard]] ConfigLoadResult resolve(
        const std::optional<std::string>& remotePayload) const;

private:
    ConfigCache cache_;
};

} // namespace proxima::backend
