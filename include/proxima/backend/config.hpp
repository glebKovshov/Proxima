#pragma once

#include "proxima/core/domain.hpp"
#include "proxima/routing/route_scoring_engine.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace proxima::backend {

struct BackendConfig {
    std::string schemaVersion{"1.0"};
    std::string minClientVersion{"0.1.0"};
    std::string latestVersion{"0.1.0"};
    bool maintenanceMode{false};
    core::GameProfile gameProfile;
    std::vector<std::string> diagnosticEndpoints{
        "fra-de-ping.vultr.com",
        "speedtest.frankfurt.linode.com",
        "ams-nl-ping.vultr.com",
        "lon-gb-ping.vultr.com",
        "par-fr-ping.vultr.com",
        "waw-pl-ping.vultr.com",
        "sto-se-ping.vultr.com",
        "mad-es-ping.vultr.com"};
    std::vector<std::string> diagnosticEndpointLabels{
        "Frankfurt №1",
        "Frankfurt №2",
        "Amsterdam №1",
        "London №1",
        "Paris №1",
        "Warsaw №1",
        "Stockholm №1",
        "Madrid №1"};
    routing::ScoringParameters scoring;
    std::vector<core::RelayRouteMetadata> relays;
};

class SchemaValidator {
public:
    [[nodiscard]] static bool validate(std::string_view payload, std::string* error = nullptr);
};

class ConfigParser {
public:
    [[nodiscard]] static std::optional<BackendConfig> parse(
        std::string_view payload,
        std::string* error = nullptr);
};

class ConfigManager {
public:
    [[nodiscard]] static BackendConfig defaultConfig();
    [[nodiscard]] static std::optional<BackendConfig> loadValidated(
        std::string_view payload,
        std::string* error = nullptr);
};

} // namespace proxima::backend
