#pragma once

#include "proxima/backend/config_cache.hpp"
#include "proxima/backend/backend_client.hpp"
#include "proxima/core/domain.hpp"
#include "proxima/discovery/endpoint_discovery.hpp"
#include "proxima/discovery/fortnite_process_detector.hpp"
#include "proxima/logging/diagnostic_exporter.hpp"
#include "proxima/logging/logger.hpp"
#include "proxima/platform/windows/windows_network_provider.hpp"
#include "proxima/platform/windows/windows_process_provider.hpp"
#include "proxima/platform/windows/windows_system_info.hpp"
#include "proxima/routing/route_scoring_engine.hpp"
#include "proxima/transport/transport.hpp"

#include <QMainWindow>
#include <QPointer>

#include <atomic>
#include <memory>

class QLabel;
class QCloseEvent;
class QComboBox;
class QFrame;
class QListWidget;
class QPushButton;
class QProgressBar;
class QNetworkAccessManager;
class QSystemTrayIcon;
class QTimer;

namespace proxima::desktop {

class PingGraph;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void configureTray();
    void configureEndpointSelector();
    void applyEndpointSelection(int index);
    void loadBackendConfig();
    void pollGameProcess();
    void requestMeasurement();
    void updateMetrics(const core::RouteMetrics& metrics);
    void runDiagnostics();
    void exportDiagnostics();
    void updateFortniteStatus(bool running, const QString& detail);
    void updateStatus(const QString& text, const QString& tone = "neutral");
    [[nodiscard]] QString metricText(double value, const QString& suffix = " ms") const;
    [[nodiscard]] logging::DiagnosticSnapshot snapshotForExport() const;

    QLabel* fortniteStatus_{nullptr};
    QLabel* networkStatus_{nullptr};
    QComboBox* endpointSelector_{nullptr};
    QLabel* routeLabel_{nullptr};
    QLabel* diagnosticsStatus_{nullptr};
    QLabel* backendStatus_{nullptr};
    QLabel* stateLabel_{nullptr};
    QLabel* loadingTitle_{nullptr};
    QLabel* loadingDetail_{nullptr};
    QLabel* metricCurrent_{nullptr};
    QLabel* metricMinimum_{nullptr};
    QLabel* metricAverage_{nullptr};
    QLabel* metricMaximum_{nullptr};
    QLabel* metricJitter_{nullptr};
    QLabel* metricLoss_{nullptr};
    QLabel* metricStability_{nullptr};
    QListWidget* routesList_{nullptr};
    PingGraph* graph_{nullptr};
    QPushButton* diagnosticsButton_{nullptr};
    QPushButton* exportButton_{nullptr};
    QProgressBar* diagnosticsProgress_{nullptr};
    QProgressBar* loadingProgress_{nullptr};
    QFrame* loadingOverlay_{nullptr};
    QTimer* processTimer_{nullptr};
    QTimer* measurementTimer_{nullptr};
    QNetworkAccessManager* networkManager_{nullptr};
    QSystemTrayIcon* tray_{nullptr};

    std::shared_ptr<std::atomic_bool> measurementInFlight_{std::make_shared<std::atomic_bool>(false)};
    std::shared_ptr<transport::DirectTransport> directTransport_;
    std::shared_ptr<discovery::IProcessProvider> processProvider_;
    std::shared_ptr<discovery::INetworkProvider> networkProvider_;
    std::unique_ptr<discovery::FortniteProcessDetector> processDetector_;
    std::unique_ptr<discovery::EndpointDiscovery> endpointDiscovery_;
    std::unique_ptr<backend::ConfigResolver> configResolver_;
    backend::BackendConfig backendConfig_;
    backend::ConfigSource configSource_{backend::ConfigSource::BuiltIn};
    std::unique_ptr<logging::Logger> logger_;
    core::RouteMetrics lastMetrics_;
    core::ApplicationState applicationState_{core::ApplicationState::Starting};
    bool fortniteRunning_{false};
    bool startupLoading_{true};
    bool closeToTray_{true};
};

} // namespace proxima::desktop
