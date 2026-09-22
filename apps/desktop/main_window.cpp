#include "main_window.hpp"

#include "proxima/diagnostics/dns_probe.hpp"
#include "proxima/diagnostics/ping_probe.hpp"
#include "proxima/diagnostics/traceroute_probe.hpp"
#include "proxima/update/update_checker.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QFileDialog>
#include <QFile>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QStackedLayout>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace proxima::desktop {

namespace {
constexpr auto kPanel = "background:#171b22;border:1px solid #293241;border-radius:16px;";
constexpr auto kMuted = "color:#8b96a8;";
constexpr auto kAccent = "#72b7ff";

QLabel* makeLabel(const QString& text, const QString& style = {}) {
    auto* label = new QLabel(text);
    label->setStyleSheet(style);
    return label;
}

QFrame* makeCard(const QString& title, QLabel*& value) {
    auto* card = new QFrame;
    card->setStyleSheet(kPanel);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 16);
    auto* caption = makeLabel(title.toUpper(), "font-size:11px;font-weight:600;color:#7f8da3;letter-spacing:1px;");
    value = makeLabel("—", "font-size:26px;font-weight:700;color:#f4f7fb;");
    layout->addWidget(caption);
    layout->addWidget(value);
    return card;
}
}

class PingGraph final : public QWidget {
public:
    explicit PingGraph(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(150);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void addValue(const double value) {
        if (value <= 0.0) return;
        values_.push_back(value);
        while (values_.size() > 120) values_.pop_front();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#12161d"));
        painter.setPen(QPen(QColor("#283241"), 1));
        for (int i = 1; i < 4; ++i) {
            const auto y = height() * i / 4;
            painter.drawLine(12, y, width() - 12, y);
        }
        if (values_.size() < 2) return;
        const auto [minimum, maximum] = std::minmax_element(values_.begin(), values_.end());
        const double low = std::max(0.0, *minimum - 5.0);
        const double high = std::max(low + 10.0, *maximum + 5.0);
        QPainterPath path;
        for (qsizetype index = 0; index < values_.size(); ++index) {
            const double x = 12.0 + (width() - 24.0) * static_cast<double>(index) /
                static_cast<double>(values_.size() - 1);
            const double y = height() - 12.0 - (height() - 24.0) *
                ((values_[index] - low) / (high - low));
            if (index == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        painter.setPen(QPen(QColor(kAccent), 2.5));
        painter.drawPath(path);
    }

private:
    QVector<double> values_;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setMinimumSize(980, 720);
    resize(1180, 820);
    setWindowTitle("Proxima — Network Diagnostics");

    backendConfig_ = backend::ConfigManager::defaultConfig();
    const auto appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    configResolver_ = std::make_unique<backend::ConfigResolver>(
        backend::ConfigCache(std::filesystem::path(appData.toStdString()) / "config.json"));
    logger_ = std::make_unique<logging::Logger>(
        std::filesystem::path(appData.toStdString()) / "logs" / "app.log");
    logger_->info("Application", "Proxima started");

    processProvider_ = std::make_shared<platform::windows::WindowsProcessProvider>();
    networkProvider_ = std::make_shared<platform::windows::WindowsNetworkProvider>();
    processDetector_ = std::make_unique<discovery::FortniteProcessDetector>(backendConfig_.gameProfile, processProvider_);
    endpointDiscovery_ = std::make_unique<discovery::EndpointDiscovery>(networkProvider_);
    directTransport_ = std::make_shared<transport::DirectTransport>(
        "direct",
        backendConfig_.diagnosticEndpoints.front(),
        std::make_shared<diagnostics::IcmpPingProbe>());
    directTransport_->start();

    buildUi();
    configureTray();

    processTimer_ = new QTimer(this);
    processTimer_->setInterval(1500);
    connect(processTimer_, &QTimer::timeout, this, &MainWindow::pollGameProcess);
    processTimer_->start();

    measurementTimer_ = new QTimer(this);
    measurementTimer_->setInterval(5000);
    connect(measurementTimer_, &QTimer::timeout, this, &MainWindow::requestMeasurement);
    measurementTimer_->start();

    networkManager_ = new QNetworkAccessManager(this);
    loadBackendConfig();
    pollGameProcess();
    requestMeasurement();
}

MainWindow::~MainWindow() {
    measurementInFlight_->store(true);
    if (directTransport_) directTransport_->stop();
    if (logger_) logger_->info("Application", "Proxima stopped");
}

void MainWindow::buildUi() {
    setStyleSheet(R"(
        QMainWindow { background:#0e1117; color:#eef3f9; }
        QWidget { font-family:"Segoe UI"; color:#eef3f9; }
        QPushButton { background:#202937; border:1px solid #334155; border-radius:10px; padding:10px 18px; font-weight:600; }
        QPushButton:hover { background:#29374a; border-color:#72b7ff; }
        QPushButton:pressed { background:#172333; }
        QListWidget { background:#171b22; border:1px solid #293241; border-radius:12px; padding:8px; }
        QListWidget::item { padding:10px; border-radius:8px; }
        QListWidget::item:selected { background:#23354a; }
        QComboBox { background:#151a22; border:1px solid #334155; border-radius:7px; padding:7px 10px; font-size:14px; font-weight:600; color:#f4f7fb; }
        QComboBox:hover { border-color:#72b7ff; }
        QComboBox::drop-down { border:0; width:26px; }
        QComboBox QAbstractItemView { background:#171b22; border:1px solid #334155; selection-background-color:#23354a; padding:4px; }
        QProgressBar { background:#151a22; border:0; border-radius:4px; height:7px; }
        QProgressBar::chunk { background:#72b7ff; border-radius:4px; }
    )");

    auto* root = new QWidget;
    auto* outer = new QVBoxLayout(root);
    outer->setContentsMargins(28, 24, 28, 24);
    outer->setSpacing(18);

    auto* header = new QHBoxLayout;
    auto* brand = new QVBoxLayout;
    auto* logo = makeLabel("PROXIMA", "font-size:25px;font-weight:800;letter-spacing:4px;color:#f4f7fb;");
    auto* subtitle = makeLabel("NETWORK DIAGNOSTICS PLATFORM", "font-size:10px;font-weight:600;letter-spacing:2px;color:#72b7ff;");
    brand->addWidget(logo);
    brand->addWidget(subtitle);
    header->addLayout(brand);
    header->addStretch();
    stateLabel_ = makeLabel("STARTING", "font-size:11px;font-weight:700;letter-spacing:1px;color:#8b96a8;padding:8px 12px;background:#171b22;border-radius:8px;");
    header->addWidget(stateLabel_, 0, Qt::AlignTop);
    outer->addLayout(header);

    auto* statusPanel = new QFrame;
    statusPanel->setStyleSheet("background:#1a2533;border:1px solid #2b4b68;border-radius:16px;");
    auto* statusLayout = new QHBoxLayout(statusPanel);
    statusLayout->setContentsMargins(22, 18, 22, 18);
    auto* gameColumn = new QVBoxLayout;
    gameColumn->addWidget(makeLabel("FORTNITE", "font-size:11px;color:#8da0b8;letter-spacing:1px;font-weight:600;"));
    fortniteStatus_ = makeLabel("Not Running", "font-size:24px;font-weight:700;color:#dce7f5;");
    gameColumn->addWidget(fortniteStatus_);
    networkStatus_ = makeLabel("Waiting for network sample", kMuted);
    gameColumn->addWidget(networkStatus_);
    statusLayout->addLayout(gameColumn);
    statusLayout->addStretch();
    auto* regionColumn = new QVBoxLayout;
    regionColumn->addWidget(makeLabel("REGION", "font-size:10px;color:#8da0b8;letter-spacing:1px;font-weight:600;"));
    regionColumn->addWidget(makeLabel("EUROPE", "font-size:18px;font-weight:700;color:#f4f7fb;"));
    regionColumn->addWidget(makeLabel("ROUTE", "font-size:10px;color:#8da0b8;letter-spacing:1px;font-weight:600;"));
    routeLabel_ = makeLabel("Direct", "font-size:18px;font-weight:700;color:#72b7ff;");
    regionColumn->addWidget(routeLabel_);
    statusLayout->addLayout(regionColumn);
    outer->addWidget(statusPanel);

    auto* metricsGrid = new QGridLayout;
    metricsGrid->setHorizontalSpacing(12);
    metricsGrid->setVerticalSpacing(12);
    metricsGrid->addWidget(makeCard("Current ping", metricCurrent_), 0, 0);
    metricsGrid->addWidget(makeCard("Minimum ping", metricMinimum_), 0, 1);
    metricsGrid->addWidget(makeCard("Average ping", metricAverage_), 0, 2);
    metricsGrid->addWidget(makeCard("Maximum ping", metricMaximum_), 0, 3);
    metricsGrid->addWidget(makeCard("Jitter", metricJitter_), 1, 0);
    metricsGrid->addWidget(makeCard("Packet loss", metricLoss_), 1, 1);
    metricsGrid->addWidget(makeCard("Stability", metricStability_), 1, 2);
    auto* endpointCard = new QFrame;
    endpointCard->setStyleSheet(kPanel);
    auto* endpointLayout = new QVBoxLayout(endpointCard);
    endpointLayout->setContentsMargins(18, 14, 18, 16);
    endpointLayout->addWidget(makeLabel("ENDPOINT", "font-size:11px;font-weight:600;color:#7f8da3;letter-spacing:1px;"));
    endpointSelector_ = new QComboBox;
    endpointSelector_->setMinimumWidth(205);
    endpointSelector_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    endpointSelector_->setMinimumContentsLength(16);
    endpointLayout->addWidget(endpointSelector_);
    connect(endpointSelector_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](const int index) {
        applyEndpointSelection(index);
    });
    metricsGrid->addWidget(endpointCard, 1, 3);
    outer->addLayout(metricsGrid);

    auto* content = new QHBoxLayout;
    content->setSpacing(16);
    auto* chartPanel = new QFrame;
    chartPanel->setStyleSheet(kPanel);
    auto* chartLayout = new QVBoxLayout(chartPanel);
    chartLayout->setContentsMargins(18, 16, 18, 16);
    auto* chartHeader = new QHBoxLayout;
    chartHeader->addWidget(makeLabel("PING HISTORY", "font-size:11px;font-weight:700;letter-spacing:1px;color:#8b96a8;"));
    chartHeader->addStretch();
    chartHeader->addWidget(makeLabel("last 120 samples", kMuted));
    chartLayout->addLayout(chartHeader);
    graph_ = new PingGraph;
    chartLayout->addWidget(graph_);
    content->addWidget(chartPanel, 3);

    auto* routesPanel = new QFrame;
    routesPanel->setStyleSheet(kPanel);
    auto* routesLayout = new QVBoxLayout(routesPanel);
    routesLayout->setContentsMargins(16, 16, 16, 16);
    routesLayout->addWidget(makeLabel("ROUTES", "font-size:11px;font-weight:700;letter-spacing:1px;color:#8b96a8;"));
    routesList_ = new QListWidget;
    routesList_->addItem("Direct   ·   waiting for metrics");
    routesLayout->addWidget(routesList_);
    routesLayout->addWidget(makeLabel("Relay routes will use the same route contract.", "font-size:11px;color:#65758a;"));
    content->addWidget(routesPanel, 2);
    outer->addLayout(content, 1);

    auto* footer = new QHBoxLayout;
    diagnosticsStatus_ = makeLabel("Diagnostics ready", kMuted);
    backendStatus_ = makeLabel("Config: built-in defaults", kMuted);
    footer->addWidget(diagnosticsStatus_);
    footer->addSpacing(18);
    footer->addWidget(backendStatus_);
    footer->addStretch();
    diagnosticsProgress_ = new QProgressBar;
    diagnosticsProgress_->setRange(0, 0);
    diagnosticsProgress_->setVisible(false);
    diagnosticsProgress_->setMaximumWidth(100);
    footer->addWidget(diagnosticsProgress_);
    diagnosticsButton_ = new QPushButton("Diagnostics");
    exportButton_ = new QPushButton("Export Diagnostics");
    auto* settingsButton = new QPushButton("Settings");
    connect(diagnosticsButton_, &QPushButton::clicked, this, &MainWindow::runDiagnostics);
    connect(exportButton_, &QPushButton::clicked, this, &MainWindow::exportDiagnostics);
    connect(settingsButton, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, "Settings", "Settings will be persisted in a future v0.1.x build.\n\nCurrent defaults: tray enabled, 5 s idle probe interval, telemetry disabled.");
    });
    footer->addWidget(diagnosticsButton_);
    footer->addWidget(exportButton_);
    footer->addWidget(settingsButton);
    outer->addLayout(footer);

    auto* canvas = new QWidget;
    auto* layers = new QStackedLayout(canvas);
    layers->setStackingMode(QStackedLayout::StackAll);
    layers->addWidget(root);

    loadingOverlay_ = new QFrame;
    loadingOverlay_->setObjectName("startupLoadingOverlay");
    loadingOverlay_->setStyleSheet(
        "QFrame#startupLoadingOverlay { background: rgba(5, 8, 14, 218); }"
        "QFrame#startupLoadingCard { background: #171b22; border: 1px solid #36516e; border-radius: 16px; }"
        "QLabel#startupLoadingTitle { color: #f4f7fb; font-size: 19px; font-weight: 800; letter-spacing: 2px; }"
        "QLabel#startupLoadingDetail { color: #9eacc0; font-size: 12px; }"
        "QProgressBar#startupLoadingProgress { background: #10151c; border: 0; border-radius: 3px; min-height: 6px; max-height: 6px; }"
        "QProgressBar#startupLoadingProgress::chunk { background: #72b7ff; border-radius: 3px; }"
    );
    auto* overlayLayout = new QVBoxLayout(loadingOverlay_);
    overlayLayout->setContentsMargins(24, 24, 24, 24);

    auto* loadingCard = new QFrame(loadingOverlay_);
    loadingCard->setObjectName("startupLoadingCard");
    loadingCard->setMaximumWidth(430);
    auto* loadingCardLayout = new QVBoxLayout(loadingCard);
    loadingCardLayout->setContentsMargins(34, 30, 34, 30);
    loadingCardLayout->setSpacing(12);
    loadingTitle_ = makeLabel("PROXIMA", {});
    loadingTitle_->setObjectName("startupLoadingTitle");
    loadingDetail_ = makeLabel("Устанавливаем соединение и вычисляем метрики…", {});
    loadingDetail_->setObjectName("startupLoadingDetail");
    loadingProgress_ = new QProgressBar;
    loadingProgress_->setObjectName("startupLoadingProgress");
    loadingProgress_->setRange(0, 0);
    loadingProgress_->setTextVisible(false);
    loadingCardLayout->addWidget(loadingTitle_);
    loadingCardLayout->addWidget(loadingDetail_);
    loadingCardLayout->addSpacing(8);
    loadingCardLayout->addWidget(loadingProgress_);
    overlayLayout->addWidget(loadingCard, 0, Qt::AlignCenter);
    layers->addWidget(loadingOverlay_);
    setCentralWidget(canvas);
    configureEndpointSelector();
}

void MainWindow::configureEndpointSelector() {
    if (!endpointSelector_) return;

    const auto previousHost = endpointSelector_->currentData().toString();
    const QSignalBlocker blocker(endpointSelector_);
    endpointSelector_->clear();

    const auto& endpoints = backendConfig_.diagnosticEndpoints;
    for (std::size_t index = 0; index < endpoints.size(); ++index) {
        const auto label = backendConfig_.diagnosticEndpointLabels.size() == endpoints.size()
            ? QString::fromStdString(backendConfig_.diagnosticEndpointLabels[index])
            : QString("Europe №%1").arg(static_cast<int>(index + 1));
        endpointSelector_->addItem(label, QString::fromStdString(endpoints[index]));
    }

    if (endpointSelector_->count() == 0) return;
    auto selectedIndex = endpointSelector_->findData(previousHost);
    if (selectedIndex < 0) selectedIndex = 0;
    endpointSelector_->setCurrentIndex(selectedIndex);
    if (directTransport_) {
        directTransport_->setEndpoint(endpointSelector_->itemData(selectedIndex).toString().toStdString());
    }
}

void MainWindow::applyEndpointSelection(const int index) {
    if (!endpointSelector_ || index < 0 || index >= endpointSelector_->count() || !directTransport_) return;
    const auto endpoint = endpointSelector_->itemData(index).toString().toStdString();
    if (endpoint.empty()) return;

    directTransport_->setEndpoint(endpoint);
    directTransport_->resetMetrics();
    startupLoading_ = true;
    loadingDetail_->setText(QString("Проверяем %1 и пересчитываем метрики…").arg(endpointSelector_->itemText(index)));
    loadingOverlay_->show();
    loadingOverlay_->raise();
    requestMeasurement();
}

void MainWindow::configureTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;
    tray_ = new QSystemTrayIcon(style()->standardIcon(QStyle::SP_ComputerIcon), this);
    auto* menu = new QMenu(this);
    auto* open = menu->addAction("Open Proxima");
    auto* status = menu->addAction("Fortnite: Not Running");
    status->setEnabled(false);
    auto* ping = menu->addAction("Ping: —");
    ping->setEnabled(false);
    menu->addSeparator();
    auto* diagnostics = menu->addAction("Diagnostics");
    menu->addSeparator();
    auto* exit = menu->addAction("Exit");
    connect(open, &QAction::triggered, this, [this] { showNormal(); activateWindow(); });
    connect(diagnostics, &QAction::triggered, this, &MainWindow::runDiagnostics);
    connect(exit, &QAction::triggered, qApp, &QApplication::quit);
    tray_->setContextMenu(menu);
    tray_->setToolTip("Proxima — network diagnostics");
    tray_->show();
}

void MainWindow::loadBackendConfig() {
    std::string backendUrl;
#ifdef _MSC_VER
    char* urlEnvironment = nullptr;
    std::size_t urlLength = 0;
    if (_dupenv_s(&urlEnvironment, &urlLength, "PROXIMA_BACKEND_URL") == 0 && urlEnvironment != nullptr) {
        backendUrl = urlEnvironment;
        free(urlEnvironment);
    }
#else
    if (const auto* urlEnvironment = std::getenv("PROXIMA_BACKEND_URL")) backendUrl = urlEnvironment;
#endif
    const QString url = QString::fromStdString(backendUrl);
    if (url.isEmpty()) {
        backendStatus_->setText("Config: built-in defaults");
        return;
    }
    if (!url.startsWith("https://")) {
        backendStatus_->setText("Config: rejected (HTTPS required)");
        logger_->warning("Backend", "Rejected non-HTTPS backend URL");
        return;
    }
    auto* reply = networkManager_->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        std::optional<std::string> payload;
        if (reply->error() == QNetworkReply::NoError && status >= 200 && status < 300) {
            payload = reply->readAll().toStdString();
        }
        const auto resolved = configResolver_->resolve(payload);
        backendConfig_ = resolved.config;
        configSource_ = resolved.source;
        processDetector_->setProfile(backendConfig_.gameProfile);
        configureEndpointSelector();
        backendStatus_->setText(QString("Config: %1").arg(
            resolved.source == backend::ConfigSource::Remote ? "remote" :
            resolved.source == backend::ConfigSource::Cache ? "cache" : "built-in"));
        if (!resolved.warning.empty()) logger_->warning("Backend", resolved.warning);
        reply->deleteLater();
    });
}

void MainWindow::pollGameProcess() {
    const auto process = processDetector_->detect();
    const bool running = process.has_value();
    if (running != fortniteRunning_ || applicationState_ == core::ApplicationState::Starting) {
        fortniteRunning_ = running;
        directTransport_->resetMetrics();
        if (running) {
            applicationState_ = core::ApplicationState::Monitoring;
            measurementTimer_->setInterval(1000);
            updateFortniteStatus(true, "Active diagnostics");
            logger_->info("Discovery", "Fortnite process detected");
        } else {
            applicationState_ = core::ApplicationState::Idle;
            measurementTimer_->setInterval(5000);
            updateFortniteStatus(false, "Diagnostic endpoint mode");
            logger_->info("Discovery", "Fortnite process not detected");
        }
    }
    if (tray_) {
        const auto actions = tray_->contextMenu()->actions();
        if (actions.size() >= 3) {
            actions.at(1)->setText(QString("Fortnite: %1").arg(fortniteRunning_ ? "Running" : "Not Running"));
            actions.at(2)->setText(QString("Ping: %1").arg(metricText(lastMetrics_.rttCurrentMs, " ms")));
        }
    }
}

void MainWindow::requestMeasurement() {
    if (!directTransport_ || measurementInFlight_->exchange(true)) return;
    const auto transport = directTransport_;
    const auto busy = measurementInFlight_;
    QPointer<MainWindow> self(this);
    QtConcurrent::run([transport, busy, self] {
        transport->measure(std::chrono::milliseconds(900));
        const auto metrics = transport->metrics();
        busy->store(false);
        if (self) {
        (void)QMetaObject::invokeMethod(self, [self, metrics] {
                if (self) self->updateMetrics(metrics);
            }, Qt::QueuedConnection);
        }
    });
}

void MainWindow::updateMetrics(const core::RouteMetrics& metrics) {
    lastMetrics_ = metrics;
    const auto scored = routing::RouteScoringEngine(backendConfig_.scoring).score(metrics);
    lastMetrics_.qualityScore = scored.qualityScore;
    lastMetrics_.confidence = scored.confidence;
    lastMetrics_.health = scored.health;
    metricCurrent_->setText(metricText(metrics.rttCurrentMs));
    metricMinimum_->setText(metricText(metrics.rttMinMs));
    metricAverage_->setText(metricText(metrics.rttAvgMs));
    metricMaximum_->setText(metricText(metrics.rttMaxMs));
    metricJitter_->setText(metricText(metrics.jitterMs));
    metricLoss_->setText(QString::number(metrics.packetLossPct, 'f', 1) + " %");
    metricStability_->setText(QString("%1 / 100").arg(QString::number(scored.qualityScore, 'f', 0)));
    metricStability_->setStyleSheet(QString("font-size:26px;font-weight:700;color:%1;").arg(
        scored.health == core::RouteHealth::Healthy ? "#6fe0a0" :
        scored.health == core::RouteHealth::Degraded ? "#f7c96b" : "#f28b8b"));
    networkStatus_->setText(QString("%1 · %2 samples · %3")
        .arg(QString::fromStdString(scored.label))
        .arg(metrics.sampleCount)
        .arg(QString::fromUtf8(core::toString(metrics.health))));
    stateLabel_->setText(QString::fromUtf8(core::toString(applicationState_)).toUpper());
    if (metrics.sampleCount > 0) graph_->addValue(metrics.rttCurrentMs);
    routesList_->item(0)->setText(QString("Direct   ·   %1   ·   %2")
        .arg(metricText(metrics.rttAvgMs))
        .arg(QString::fromStdString(scored.label)));
    if (startupLoading_ && (metrics.sampleCount > 0 || metrics.packetLossPct > 0.0)) {
        startupLoading_ = false;
        loadingOverlay_->hide();
    }
}

void MainWindow::runDiagnostics() {
    if (diagnosticsProgress_->isVisible()) return;
    diagnosticsProgress_->setVisible(true);
    diagnosticsButton_->setEnabled(false);
    diagnosticsStatus_->setText("Running traceroute and DNS diagnostics…");
    const auto host = directTransport_->endpoint();
    QPointer<MainWindow> self(this);
    [[maybe_unused]] const auto future = QtConcurrent::run([self, host] {
        const auto hops = diagnostics::TracerouteProbe{}.run(host);
        const auto dns = diagnostics::DnsProbe{}.resolve(host);
        const auto traceText = logging::DiagnosticExporter::tracerouteToText(hops);
        const auto dnsText = QString("DNS %1 · %2 ms · %3 addresses")
            .arg(dns.success ? "OK" : "FAILED")
            .arg(dns.resolveLatencyMs.value_or(0.0), 0, 'f', 1)
            .arg(dns.addresses.size());
        if (self) {
            (void)QMetaObject::invokeMethod(self, [self, traceText, dnsText] {
                if (!self) return;
                self->diagnosticsProgress_->setVisible(false);
                self->diagnosticsButton_->setEnabled(true);
                self->diagnosticsStatus_->setText(dnsText);
                self->logger_->info("Diagnostics", "Traceroute and DNS diagnostics completed");
                QMessageBox::information(self, "Diagnostics", QString::fromStdString(traceText).left(4000));
            }, Qt::QueuedConnection);
        }
    });
}

void MainWindow::exportDiagnostics() {
    const auto directory = QFileDialog::getExistingDirectory(this, "Export diagnostics", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (directory.isEmpty()) return;
    const auto snapshot = snapshotForExport();
    QPointer<MainWindow> self(this);
    [[maybe_unused]] const auto future = QtConcurrent::run([self, directory, snapshot] {
        std::string error;
        const auto result = logging::DiagnosticExporter::exportZip(
            std::filesystem::path(directory.toStdString()), snapshot, &error);
        if (self) {
            (void)QMetaObject::invokeMethod(self, [self, result, error] {
                if (!self) return;
                if (result) {
                    self->diagnosticsStatus_->setText("Diagnostics exported");
                    QMessageBox::information(self, "Export complete", QString("Archive created:\n%1").arg(QString::fromStdString(result->string())));
                } else {
                    QMessageBox::warning(self, "Export failed", QString::fromStdString(error));
                }
            }, Qt::QueuedConnection);
        }
    });
}

void MainWindow::updateFortniteStatus(const bool running, const QString& detail) {
    fortniteStatus_->setText(running ? "Running" : "Not Running");
    fortniteStatus_->setStyleSheet(QString("font-size:24px;font-weight:700;color:%1;").arg(running ? "#6fe0a0" : "#dce7f5"));
    networkStatus_->setText(detail);
}

void MainWindow::updateStatus(const QString& text, const QString& tone) {
    (void)tone;
    diagnosticsStatus_->setText(text);
}

QString MainWindow::metricText(const double value, const QString& suffix) const {
    return value <= 0.0 ? "—" : QString::number(value, 'f', value < 10.0 ? 1 : 0) + suffix;
}

logging::DiagnosticSnapshot MainWindow::snapshotForExport() const {
    logging::DiagnosticSnapshot snapshot;
    if (logger_ && !logger_->filePath().empty()) {
        QFile file(QString::fromStdString(logger_->filePath().string()));
        if (file.open(QIODevice::ReadOnly)) snapshot.appLog = file.readAll().toStdString();
    }
    snapshot.networkJson = logging::DiagnosticExporter::metricsToJson(lastMetrics_);
    snapshot.routesJson = "[{\"id\":\"direct\",\"type\":\"Direct\",\"futureRelayReady\":true}]";
    snapshot.tracerouteText = "Run Diagnostics to include the latest traceroute.\n";
    const auto system = platform::windows::WindowsSystemInfo::collect();
    snapshot.systemJson = QString("{\"os\":\"%1\",\"architecture\":\"%2\",\"memoryMb\":%3}")
        .arg(QString::fromStdString(system.operatingSystem))
        .arg(QString::fromStdString(system.architecture))
        .arg(system.memoryMb).toStdString();
    snapshot.versionText = QString("Proxima %1\n").arg(QString::fromUtf8(PROXIMA_VERSION)).toStdString();
    return snapshot;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (closeToTray_ && tray_ && tray_->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    event->accept();
}

} // namespace proxima::desktop
