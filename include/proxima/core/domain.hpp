#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace proxima::core {

enum class RouteType { Direct, Relay };
enum class RouteHealth { Unknown, Healthy, Degraded, Offline };
enum class MeasurementMode { Idle, Active, BurstTest };
enum class ApplicationState {
    Starting,
    Idle,
    FortniteDetected,
    Monitoring,
    DiagnosticsRunning,
    BackendOffline,
    UpdateAvailable,
    Error
};

struct Endpoint {
    std::string address;
    std::uint16_t port{0};
    std::string protocol{"unknown"};
    std::chrono::system_clock::time_point lastSeen{};
    std::uint64_t activityCount{0};
    double activityPerSecond{0.0};
    bool likelyGameEndpoint{false};

    [[nodiscard]] std::string displayAddress() const;
};

struct GameProfile {
    std::string gameId{"fortnite"};
    std::string displayName{"Fortnite"};
    std::vector<std::string> executableNames{
        "FortniteClient-Win64-Shipping.exe",
        "FortniteClient-Win64-Shipping_BE.exe",
        "FortniteClient-Win64-Shipping_EAC.exe"
    };
    std::string region{"europe"};
};

struct NetworkSample {
    std::optional<double> rttMs;
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};

    [[nodiscard]] bool successful() const noexcept { return rttMs.has_value(); }
};

struct RouteMetrics {
    std::string routeId{"direct"};
    RouteType routeType{RouteType::Direct};
    double rttCurrentMs{0.0};
    double rttMinMs{0.0};
    double rttAvgMs{0.0};
    double rttMaxMs{0.0};
    double jitterMs{0.0};
    double packetLossPct{100.0};
    double stabilityScore{0.0};
    std::size_t sampleCount{0};
    std::chrono::system_clock::time_point lastUpdated{};
    RouteHealth health{RouteHealth::Unknown};
    double confidence{0.0};
    double qualityScore{0.0};
};

struct RelayRouteMetadata {
    std::string relayId;
    std::string relayRegion;
    std::string relayAddress;
    std::uint16_t relayPort{0};
    std::string protocol{"udp"};
    RouteHealth health{RouteHealth::Unknown};
    double load{0.0};
};

struct Route {
    std::string id{"direct"};
    RouteType type{RouteType::Direct};
    std::string displayName{"Direct"};
    std::string region{"europe"};
    std::optional<Endpoint> endpoint;
    std::optional<RelayRouteMetadata> relay;
    RouteMetrics metrics;
};

[[nodiscard]] const char* toString(RouteType value) noexcept;
[[nodiscard]] const char* toString(RouteHealth value) noexcept;
[[nodiscard]] const char* toString(MeasurementMode value) noexcept;
[[nodiscard]] const char* toString(ApplicationState value) noexcept;

} // namespace proxima::core
