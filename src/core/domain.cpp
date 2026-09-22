#include "proxima/core/domain.hpp"

#include <string>

namespace proxima::core {

std::string Endpoint::displayAddress() const {
    if (port == 0) {
        return address;
    }
    return address + ":" + std::to_string(port);
}

const char* toString(const RouteType value) noexcept {
    switch (value) {
    case RouteType::Direct: return "Direct";
    case RouteType::Relay: return "Relay";
    }
    return "Unknown";
}

const char* toString(const RouteHealth value) noexcept {
    switch (value) {
    case RouteHealth::Unknown: return "Unknown";
    case RouteHealth::Healthy: return "Healthy";
    case RouteHealth::Degraded: return "Degraded";
    case RouteHealth::Offline: return "Offline";
    }
    return "Unknown";
}

const char* toString(const MeasurementMode value) noexcept {
    switch (value) {
    case MeasurementMode::Idle: return "Idle";
    case MeasurementMode::Active: return "Active";
    case MeasurementMode::BurstTest: return "BurstTest";
    }
    return "Idle";
}

const char* toString(const ApplicationState value) noexcept {
    switch (value) {
    case ApplicationState::Starting: return "Starting";
    case ApplicationState::Idle: return "Idle";
    case ApplicationState::FortniteDetected: return "FortniteDetected";
    case ApplicationState::Monitoring: return "Monitoring";
    case ApplicationState::DiagnosticsRunning: return "DiagnosticsRunning";
    case ApplicationState::BackendOffline: return "BackendOffline";
    case ApplicationState::UpdateAvailable: return "UpdateAvailable";
    case ApplicationState::Error: return "Error";
    }
    return "Error";
}

} // namespace proxima::core
