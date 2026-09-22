#include "proxima/backend/config_cache.hpp"
#include "proxima/diagnostics/network_measurement_engine.hpp"
#include "proxima/diagnostics/statistics.hpp"
#include "proxima/discovery/endpoint_discovery.hpp"
#include "proxima/discovery/fortnite_process_detector.hpp"
#include "proxima/logging/diagnostic_exporter.hpp"
#include "proxima/routing/route_manager.hpp"
#include "proxima/routing/route_scoring_engine.hpp"
#include "proxima/update/update_checker.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {
int failures = 0;

void expect(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expectNear(const double actual, const double expected, const double epsilon, const std::string& message) {
    expect(std::abs(actual - expected) <= epsilon, message);
}

class MockProcessProvider final : public proxima::discovery::IProcessProvider {
public:
    std::vector<proxima::discovery::ProcessInfo> processes;
    [[nodiscard]] std::vector<proxima::discovery::ProcessInfo> listProcesses() const override { return processes; }
};

class MockNetworkProvider final : public proxima::discovery::INetworkProvider {
public:
    std::vector<proxima::core::Endpoint> endpoints;
    [[nodiscard]] std::vector<proxima::core::Endpoint> endpointsForProcess(std::uint32_t) const override { return endpoints; }
};

void testStatistics() {
    proxima::diagnostics::RollingStatistics statistics(3);
    statistics.add(10.0);
    statistics.add(20.0);
    statistics.add(30.0);
    statistics.add(40.0);
    expect(statistics.size() == 3, "rolling statistics keeps capacity");
    expectNear(*statistics.min(), 20.0, 0.001, "rolling minimum");
    expectNear(*statistics.mean(), 30.0, 0.001, "rolling average");
    expectNear(proxima::diagnostics::JitterCalculator::calculate({25, 26, 24, 27, 25}), 2.0, 0.001, "jitter is mean adjacent delta");
    proxima::diagnostics::PacketLossAnalyzer loss;
    loss.record(true);
    loss.record(false);
    loss.record(true);
    loss.record(false);
    expectNear(loss.lossPercent(), 50.0, 0.001, "packet loss percentage");
}

void testMeasurementEngine() {
    proxima::diagnostics::NetworkMeasurementEngine engine;
    engine.start(proxima::core::MeasurementMode::Active);
    engine.record({25.0});
    engine.record({27.0});
    engine.record({std::nullopt});
    const auto metrics = engine.snapshot();
    expect(metrics.sampleCount == 2, "measurement engine counts successful samples");
    expectNear(metrics.packetLossPct, 33.333, 0.01, "measurement packet loss");
    expectNear(metrics.rttMinMs, 25.0, 0.001, "measurement minimum");
}

void testScoring() {
    proxima::routing::RouteScoringEngine scoring;
    proxima::core::RouteMetrics offline;
    offline.packetLossPct = 100.0;
    const auto offlineScore = scoring.score(offline);
    expect(offlineScore.qualityScore == 0.0, "offline route has no quality score without successful samples");
    expect(offlineScore.health == proxima::core::RouteHealth::Offline, "offline route is not presented as partially healthy");

    proxima::core::RouteMetrics fast;
    fast.rttAvgMs = 20;
    fast.jitterMs = 1;
    fast.packetLossPct = 0;
    fast.stabilityScore = 98;
    fast.sampleCount = 100;
    proxima::core::RouteMetrics slow = fast;
    slow.routeId = "slow";
    slow.rttAvgMs = 25;
    expect(scoring.score(fast).qualityScore > scoring.score(slow).qualityScore, "lower RTT wins equal stability case");

    auto unstable = fast;
    unstable.rttAvgMs = 20;
    unstable.jitterMs = 20;
    unstable.packetLossPct = 3;
    unstable.stabilityScore = 40;
    auto stable = fast;
    stable.rttAvgMs = 27;
    stable.jitterMs = 1;
    stable.packetLossPct = 0;
    stable.stabilityScore = 98;
    expect(scoring.score(stable).qualityScore > scoring.score(unstable).qualityScore, "stability and loss can outweigh RTT");

    auto lowConfidence = fast;
    lowConfidence.sampleCount = 2;
    expect(scoring.score(lowConfidence).confidence < 0.2, "small sample count lowers confidence");

    proxima::routing::RouteManager manager;
    proxima::core::Route first;
    first.id = "first";
    first.metrics = fast;
    proxima::core::Route second;
    second.id = "second";
    second.metrics = slow;
    manager.setRoutes({first, second});
    expect(manager.bestRoute()->id == "first", "best route selector chooses highest score");
}

void testConfigAndCache() {
    const std::string payload = R"({
        "schemaVersion":"1.0",
        "minClientVersion":"0.1.0",
        "latestVersion":"0.1.0",
        "maintenanceMode":false,
        "gameId":"fortnite",
        "displayName":"Fortnite",
        "region":"europe",
        "executableNames":["FortniteClient-Win64-Shipping.exe"],
        "diagnosticEndpoints":["example.com"],
        "rttWeight":0.5,
        "jitterWeight":0.2,
        "packetLossWeight":0.2,
        "stabilityWeight":0.1
    })";
    std::string error;
    const auto config = proxima::backend::ConfigManager::loadValidated(payload, &error);
    expect(config.has_value(), "valid backend config parses");
    expect(config && config->diagnosticEndpoints.front() == "example.com", "backend endpoint is parsed");
    expect(config && config->scoring.rttWeight == 0.5, "backend scoring parameters are parsed");
    expect(!proxima::backend::ConfigManager::loadValidated("not json", &error), "malformed backend config is rejected");

    const auto temp = std::filesystem::temp_directory_path() / "proxima-test-cache";
    std::error_code ignored;
    std::filesystem::remove_all(temp, ignored);
    proxima::backend::ConfigCache cache(temp / "config.json");
    expect(cache.write(payload), "config cache writes atomically");
    proxima::backend::ConfigResolver resolver(cache);
    const auto result = resolver.resolve(std::nullopt);
    expect(result.source == proxima::backend::ConfigSource::Cache, "cache fallback is selected");
    std::filesystem::remove_all(temp, ignored);

    expect(proxima::backend::ConfigManager::defaultConfig().diagnosticEndpoints.front() == "one.one.one.one",
           "built-in diagnostic endpoint is resolvable");
}

void testDiscovery() {
    auto processes = std::make_shared<MockProcessProvider>();
    processes->processes.push_back({1234, "FortniteClient-Win64-Shipping.exe"});
    proxima::discovery::FortniteProcessDetector detector(proxima::core::GameProfile{}, processes);
    expect(detector.isRunning(), "Fortnite process detector matches configured executable");

    auto network = std::make_shared<MockNetworkProvider>();
    network->endpoints.push_back({"127.0.0.1", 10809, "tcp", {}, 100, 10.0, true});
    network->endpoints.push_back({"1.2.3.4", 443, "tcp", {}, 1, 0.0, false});
    network->endpoints.push_back({"5.6.7.8", 9000, "udp", {}, 5, 1.0, true});
    const auto endpoints = proxima::discovery::EndpointDiscovery(network).discover(1234);
    expect(!endpoints.empty() && endpoints.front().address == "5.6.7.8", "endpoint discovery prioritizes likely game endpoint");
}

void testUpdateAndExport() {
    expect(proxima::update::UpdateChecker::isNewer("0.1.0", "0.2.0"), "newer update is detected");
    expect(!proxima::update::UpdateChecker::isNewer("0.2.0", "0.1.0"), "older update is ignored");
    expect(proxima::update::UpdateChecker::parseManifest(R"({"latestVersion":"0.2.0","downloadUrl":"https://example.com/a"})").has_value(), "update manifest parses");

    const auto temp = std::filesystem::temp_directory_path() / "proxima-test-export";
    std::error_code ignored;
    std::filesystem::remove_all(temp, ignored);
    proxima::logging::DiagnosticSnapshot snapshot;
    snapshot.networkJson = "{}";
    snapshot.versionText = "Proxima 0.1.0";
    const auto archive = proxima::logging::DiagnosticExporter::exportZip(temp, snapshot);
    expect(archive.has_value(), "diagnostic archive is created");
    if (archive) {
        std::ifstream stream(*archive, std::ios::binary);
        char signature[2]{};
        stream.read(signature, 2);
        expect(signature[0] == 'P' && signature[1] == 'K', "diagnostic archive is a ZIP");
    }
    std::filesystem::remove_all(temp, ignored);
}
}

int main() {
    testStatistics();
    testMeasurementEngine();
    testScoring();
    testConfigAndCache();
    testDiscovery();
    testUpdateAndExport();
    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed.\n";
        return 1;
    }
    std::cout << "All Proxima tests passed.\n";
    return 0;
}
