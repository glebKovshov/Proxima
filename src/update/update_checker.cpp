#include "proxima/update/update_checker.hpp"

#include <charconv>
#include <string_view>
#include <vector>

namespace proxima::update {

namespace {
std::vector<int> versionParts(const std::string& value) {
    std::vector<int> result;
    std::size_t begin = 0;
    while (begin < value.size()) {
        const auto end = value.find('.', begin);
        const auto stop = end == std::string::npos ? value.size() : end;
        int part = 0;
        std::from_chars(value.data() + begin, value.data() + stop, part);
        result.push_back(part);
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    while (result.size() < 3) result.push_back(0);
    return result;
}

std::optional<std::string> stringField(const std::string& payload, const std::string& key) {
    const auto position = payload.find('"' + key + '"');
    if (position == std::string::npos) return std::nullopt;
    const auto opening = payload.find('"', payload.find(':', position) + 1);
    if (opening == std::string::npos) return std::nullopt;
    const auto closing = payload.find('"', opening + 1);
    if (closing == std::string::npos) return std::nullopt;
    return payload.substr(opening + 1, closing - opening - 1);
}
}

bool UpdateChecker::isNewer(const std::string& current, const std::string& candidate) {
    return versionParts(candidate) > versionParts(current);
}

std::optional<UpdateManifest> UpdateChecker::parseManifest(const std::string& payload) {
    const auto version = stringField(payload, "latestVersion");
    const auto url = stringField(payload, "downloadUrl");
    if (!version || !url) return std::nullopt;
    UpdateManifest manifest{*version, *url};
    if (const auto checksum = stringField(payload, "sha256")) manifest.sha256 = *checksum;
    return manifest;
}

} // namespace proxima::update
