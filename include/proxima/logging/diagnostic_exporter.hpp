#pragma once

#include "proxima/core/domain.hpp"
#include "proxima/diagnostics/traceroute_probe.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace proxima::logging {

struct DiagnosticSnapshot {
    std::string appLog;
    std::string networkJson;
    std::string routesJson;
    std::string tracerouteText;
    std::string systemJson;
    std::string versionText;
};

class DiagnosticExporter {
public:
    [[nodiscard]] static std::optional<std::filesystem::path> exportZip(
        const std::filesystem::path& directory,
        const DiagnosticSnapshot& snapshot,
        std::string* error = nullptr);

    [[nodiscard]] static std::string tracerouteToText(
        const std::vector<diagnostics::TracerouteHop>& hops);
    [[nodiscard]] static std::string metricsToJson(const core::RouteMetrics& metrics);
};

} // namespace proxima::logging
