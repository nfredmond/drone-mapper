#include "SimulationPreviewWidget.h"
#include <QGridLayout>
#include <QCloseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QtMath>

namespace DroneMapper {
namespace UI {

SimulationPreviewWidget::SimulationPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_simulator(new Core::MissionSimulator(this))
    , m_isPlaying(false)
{
    setupUI();

    // Connect simulator signals
    connect(m_simulator, &Core::MissionSimulator::stateUpdated,
            this, &SimulationPreviewWidget::onSimulationStateUpdated);
    connect(m_simulator, &Core::MissionSimulator::waypointReached,
            this, &SimulationPreviewWidget::onWaypointReached);
    connect(m_simulator, &Core::MissionSimulator::photoCaptured,
            this, &SimulationPreviewWidget::onPhotoCaptured);
    connect(m_simulator, &Core::MissionSimulator::batteryChangeRequired,
            this, &SimulationPreviewWidget::onBatteryChangeRequired);
    connect(m_simulator, &Core::MissionSimulator::simulationCompleted,
            this, &SimulationPreviewWidget::onSimulationCompleted);
    connect(m_simulator, &Core::MissionSimulator::warning,
            this, &SimulationPreviewWidget::onWarning);

    setWindowTitle("Mission Simulation Preview");
    resize(1000, 700);
}

SimulationPreviewWidget::~SimulationPreviewWidget()
{
}

void SimulationPreviewWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);

    // Visualization area
    m_visualizationLabel = new QLabel(this);
    m_visualizationLabel->setMinimumSize(800, 400);
    m_visualizationLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_visualizationLabel->setAlignment(Qt::AlignCenter);
    m_visualizationLabel->setText("Load a flight plan to begin simulation");
    m_mainLayout->addWidget(m_visualizationLabel);

    // Control panel
    m_controlGroup = new QGroupBox("Playback Controls", this);
    QHBoxLayout *controlLayout = new QHBoxLayout();

    m_playPauseButton = new QPushButton("Play", this);
    m_playPauseButton->setEnabled(false);
    connect(m_playPauseButton, &QPushButton::clicked, this, &SimulationPreviewWidget::onPlayPauseClicked);

    m_stopButton = new QPushButton("Stop", this);
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &SimulationPreviewWidget::onStopClicked);

    m_speedLabel = new QLabel("Speed: 1.0x", this);
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setMinimum(1);
    m_speedSlider->setMaximum(100);  // 0.1x to 10.0x
    m_speedSlider->setValue(10);  // 1.0x
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    m_speedSlider->setTickInterval(10);
    connect(m_speedSlider, &QSlider::valueChanged, this, &SimulationPreviewWidget::onSpeedChanged);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);

    controlLayout->addWidget(m_playPauseButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addWidget(new QLabel("Speed:", this));
    controlLayout->addWidget(m_speedSlider);
    controlLayout->addWidget(m_speedLabel);
    controlLayout->addStretch();

    m_controlGroup->setLayout(controlLayout);
    m_mainLayout->addWidget(m_controlGroup);
    m_mainLayout->addWidget(m_progressBar);

    // Create horizontal layout for stats panels
    QHBoxLayout *panelsLayout = new QHBoxLayout();

    // Statistics panel
    m_statsGroup = new QGroupBox("Mission Statistics", this);
    QGridLayout *statsLayout = new QGridLayout();

    m_totalDistanceLabel = new QLabel("Distance: --", this);
    m_totalTimeLabel = new QLabel("Flight Time: --", this);
    m_totalPhotosLabel = new QLabel("Photos: --", this);
    m_batteryCountLabel = new QLabel("Batteries: --", this);
    m_surveyAreaLabel = new QLabel("Area: --", this);

    statsLayout->addWidget(m_totalDistanceLabel, 0, 0);
    statsLayout->addWidget(m_totalTimeLabel, 1, 0);
    statsLayout->addWidget(m_totalPhotosLabel, 2, 0);
    statsLayout->addWidget(m_batteryCountLabel, 3, 0);
    statsLayout->addWidget(m_surveyAreaLabel, 4, 0);

    m_statsGroup->setLayout(statsLayout);
    panelsLayout->addWidget(m_statsGroup);

    // Current state panel
    m_stateGroup = new QGroupBox("Current State", this);
    QGridLayout *stateLayout = new QGridLayout();

    m_currentWaypointLabel = new QLabel("Waypoint: 0/0", this);
    m_currentPositionLabel = new QLabel("Position: --", this);
    m_currentAltitudeLabel = new QLabel("Altitude: --", this);
    m_currentSpeedLabel = new QLabel("Speed: --", this);
    m_currentBatteryLabel = new QLabel("Battery: 100%", this);
    m_photosTakenLabel = new QLabel("Photos: 0", this);
    m_elapsedTimeLabel = new QLabel("Time: 00:00", this);

    m_batteryBar = new QProgressBar(this);
    m_batteryBar->setMinimum(0);
    m_batteryBar->setMaximum(100);
    m_batteryBar->setValue(100);
    m_batteryBar->setFormat("Battery: %p%");

    stateLayout->addWidget(m_currentWaypointLabel, 0, 0);
    stateLayout->addWidget(m_currentPositionLabel, 1, 0);
    stateLayout->addWidget(m_currentAltitudeLabel, 2, 0);
    stateLayout->addWidget(m_currentSpeedLabel, 3, 0);
    stateLayout->addWidget(m_batteryBar, 4, 0);
    stateLayout->addWidget(m_photosTakenLabel, 5, 0);
    stateLayout->addWidget(m_elapsedTimeLabel, 6, 0);

    m_stateGroup->setLayout(stateLayout);
    panelsLayout->addWidget(m_stateGroup);

    m_mainLayout->addLayout(panelsLayout);

    // Warnings panel
    m_warningsGroup = new QGroupBox("Validation & Warnings", this);
    QVBoxLayout *warningsLayout = new QVBoxLayout();

    m_warningsText = new QTextEdit(this);
    m_warningsText->setReadOnly(true);
    m_warningsText->setMaximumHeight(120);
    m_warningsText->setPlaceholderText("Validation warnings will appear here...");

    warningsLayout->addWidget(m_warningsText);
    m_warningsGroup->setLayout(warningsLayout);
    m_mainLayout->addWidget(m_warningsGroup);
}

void SimulationPreviewWidget::loadFlightPlan(const Models::FlightPlan& plan)
{
    m_flightPlan = plan;

    if (m_simulator->loadFlightPlan(plan)) {
        m_playPauseButton->setEnabled(true);
        m_stopButton->setEnabled(true);

        updateStatisticsDisplay();
        drawFlightPath();

        // Display initial state
        onSimulationStateUpdated(m_simulator->currentState());
    }
}

void SimulationPreviewWidget::onPlayPauseClicked()
{
    if (m_isPlaying) {
        m_simulator->pause();
        m_playPauseButton->setText("Play");
        m_isPlaying = false;
    } else {
        m_simulator->start();
        m_playPauseButton->setText("Pause");
        m_isPlaying = true;
    }
}

void SimulationPreviewWidget::onStopClicked()
{
    m_simulator->stop();
    m_playPauseButton->setText("Play");
    m_isPlaying = false;
    m_progressBar->setValue(0);
}

void SimulationPreviewWidget::onSpeedChanged(int value)
{
    double speed = value / 10.0;  // 1-100 -> 0.1x-10.0x
    m_simulator->setSimulationSpeed(speed);
    m_speedLabel->setText(QString("Speed: %1x").arg(speed, 0, 'f', 1));
}

void SimulationPreviewWidget::onSimulationStateUpdated(const Core::SimulationState& state)
{
    updateStateDisplay(state);

    // Update progress bar
    int progress = static_cast<int>(m_simulator->progress() * 100.0);
    m_progressBar->setValue(progress);

    // Redraw flight path with current position
    drawFlightPath();
}

void SimulationPreviewWidget::onWaypointReached(int waypointIndex)
{
    // Update waypoint label with highlight
    const auto& stats = m_simulator->statistics();
    m_currentWaypointLabel->setText(
        QString("<b>Waypoint: %1/%2</b>").arg(waypointIndex + 1).arg(stats.totalWaypoints));
}

void SimulationPreviewWidget::onPhotoCaptured(int photoNumber, const Models::GeospatialCoordinate& position)
{
    // Flash photo indicator
    m_photosTakenLabel->setText(QString("<b style='color: green;'>Photo #%1 captured</b>").arg(photoNumber));
}

void SimulationPreviewWidget::onBatteryChangeRequired(int batteryNumber)
{
    m_warningsText->append(QString("<span style='color: orange;'><b>Battery change required: Battery #%1</b></span>")
        .arg(batteryNumber));
}

void SimulationPreviewWidget::onSimulationCompleted(const Core::SimulationStatistics& statistics)
{
    m_playPauseButton->setText("Play");
    m_isPlaying = false;
    m_progressBar->setValue(100);

    m_warningsText->append("<span style='color: green;'><b>Simulation completed successfully!</b></span>");
    emit simulationFinished();
}

void SimulationPreviewWidget::onWarning(const QString& message)
{
    m_warningsText->append(QString("<span style='color: red;'>⚠ %1</span>").arg(message));
}

void SimulationPreviewWidget::updateStatisticsDisplay()
{
    const auto& stats = m_simulator->statistics();

    m_totalDistanceLabel->setText(QString("Distance: %1").arg(formatDistance(stats.totalDistance)));
    m_totalTimeLabel->setText(QString("Flight Time: %1").arg(formatTime(stats.totalFlightTime)));
    m_totalPhotosLabel->setText(QString("Photos: %1").arg(stats.totalPhotoCount));
    m_batteryCountLabel->setText(QString("Batteries: %1").arg(stats.batteryChangesRequired + 1));

    if (stats.surveyAreaCovered > 0) {
        double areaHa = stats.surveyAreaCovered / 10000.0;  // m² to hectares
        m_surveyAreaLabel->setText(QString("Area: %1 ha").arg(areaHa, 0, 'f', 2));
    } else {
        m_surveyAreaLabel->setText("Area: N/A");
    }
}

void SimulationPreviewWidget::updateStateDisplay(const Core::SimulationState& state)
{
    const auto& stats = m_simulator->statistics();

    m_currentWaypointLabel->setText(QString("Waypoint: %1/%2")
        .arg(state.currentWaypointIndex + 1).arg(stats.totalWaypoints));

    m_currentPositionLabel->setText(QString("Position: %1, %2")
        .arg(state.currentPosition.latitude(), 0, 'f', 6)
        .arg(state.currentPosition.longitude(), 0, 'f', 6));

    m_currentAltitudeLabel->setText(QString("Altitude: %1 m").arg(state.currentAltitude, 0, 'f', 1));
    m_currentSpeedLabel->setText(QString("Speed: %1 m/s").arg(state.currentSpeed, 0, 'f', 1));

    int batteryPercent = static_cast<int>(state.currentBatteryPercent);
    m_batteryBar->setValue(batteryPercent);

    // Color code battery level
    if (batteryPercent > 50) {
        m_batteryBar->setStyleSheet("QProgressBar::chunk { background-color: green; }");
    } else if (batteryPercent > 20) {
        m_batteryBar->setStyleSheet("QProgressBar::chunk { background-color: orange; }");
    } else {
        m_batteryBar->setStyleSheet("QProgressBar::chunk { background-color: red; }");
    }

    m_photosTakenLabel->setText(QString("Photos: %1").arg(state.photosTaken));
    m_elapsedTimeLabel->setText(QString("Time: %1").arg(formatTime(state.elapsedTime)));
}

void SimulationPreviewWidget::drawFlightPath()
{
    const auto& waypoints = m_flightPlan.waypoints();
    if (waypoints.isEmpty()) {
        return;
    }

    int width = m_visualizationLabel->width();
    int height = m_visualizationLabel->height();

    if (width < 100 || height < 100) {
        return;
    }

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Find bounds
    double minLat = waypoints[0].coordinate().latitude();
    double maxLat = waypoints[0].coordinate().latitude();
    double minLon = waypoints[0].coordinate().longitude();
    double maxLon = waypoints[0].coordinate().longitude();

    for (const auto& wp : waypoints) {
        minLat = qMin(minLat, wp.coordinate().latitude());
        maxLat = qMax(maxLat, wp.coordinate().latitude());
        minLon = qMin(minLon, wp.coordinate().longitude());
        maxLon = qMax(maxLon, wp.coordinate().longitude());
    }

    // Add padding
    double latRange = maxLat - minLat;
    double lonRange = maxLon - minLon;
    double padding = 0.1;

    minLat -= latRange * padding;
    maxLat += latRange * padding;
    minLon -= lonRange * padding;
    maxLon += lonRange * padding;

    latRange = maxLat - minLat;
    lonRange = maxLon - minLon;

    // Draw grid
    painter.setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    for (int i = 0; i <= 10; i++) {
        int x = static_cast<int>(i * width / 10);
        int y = static_cast<int>(i * height / 10);
        painter.drawLine(x, 0, x, height);
        painter.drawLine(0, y, width, y);
    }

    // Draw flight path
    painter.setPen(QPen(QColor(100, 100, 255), 2));

    for (int i = 0; i < waypoints.size() - 1; i++) {
        double x1 = (waypoints[i].coordinate().longitude() - minLon) / lonRange * width;
        double y1 = height - (waypoints[i].coordinate().latitude() - minLat) / latRange * height;
        double x2 = (waypoints[i + 1].coordinate().longitude() - minLon) / lonRange * width;
        double y2 = height - (waypoints[i + 1].coordinate().latitude() - minLat) / latRange * height;

        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    // Draw waypoints
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QBrush(QColor(100, 200, 100)));

    for (int i = 0; i < waypoints.size(); i++) {
        double x = (waypoints[i].coordinate().longitude() - minLon) / lonRange * width;
        double y = height - (waypoints[i].coordinate().latitude() - minLat) / latRange * height;

        painter.drawEllipse(QPointF(x, y), 4, 4);
    }

    // Draw current position
    const auto& state = m_simulator->currentState();
    if (state.currentWaypointIndex < waypoints.size()) {
        double x = (state.currentPosition.longitude() - minLon) / lonRange * width;
        double y = height - (state.currentPosition.latitude() - minLat) / latRange * height;

        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(QBrush(Qt::red));
        painter.drawEllipse(QPointF(x, y), 8, 8);

        // Draw direction arrow
        if (state.currentWaypointIndex < waypoints.size() - 1) {
            const auto& nextWP = waypoints[state.currentWaypointIndex + 1].coordinate();
            double x2 = (nextWP.longitude() - minLon) / lonRange * width;
            double y2 = height - (nextWP.latitude() - minLat) / latRange * height;

            double dx = x2 - x;
            double dy = y2 - y;
            double len = qSqrt(dx * dx + dy * dy);

            if (len > 0) {
                dx /= len;
                dy /= len;
                painter.drawLine(QPointF(x, y), QPointF(x + dx * 20, y + dy * 20));
            }
        }
    }

    painter.end();

    m_visualizationLabel->setPixmap(pixmap);
}

QString SimulationPreviewWidget::formatTime(int seconds) const
{
    int mins = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

QString SimulationPreviewWidget::formatDistance(double meters) const
{
    if (meters >= 1000.0) {
        return QString("%1 km").arg(meters / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 m").arg(meters, 0, 'f', 0);
    }
}

void SimulationPreviewWidget::closeEvent(QCloseEvent *event)
{
    if (m_isPlaying) {
        m_simulator->stop();
    }
    emit closed();
    event->accept();
}

} // namespace UI
} // namespace DroneMapper
