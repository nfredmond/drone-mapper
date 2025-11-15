#include "MissionSimulator.h"
#include "geospatial/GeoUtils.h"
#include <QtMath>
#include <QDebug>

namespace DroneMapper {
namespace Core {

// SimulationState implementation

SimulationState::SimulationState()
    : currentWaypointIndex(0)
    , currentAltitude(0.0)
    , currentSpeed(0.0)
    , currentBatteryPercent(100.0)
    , elapsedTime(0)
    , photosTaken(0)
    , distanceTraveled(0.0)
    , isComplete(false)
{
}

// SimulationStatistics implementation

SimulationStatistics::SimulationStatistics()
    : totalWaypoints(0)
    , totalDistance(0.0)
    , totalFlightTime(0)
    , totalPhotoCount(0)
    , surveyAreaCovered(0.0)
    , batteryChangesRequired(0)
    , maxAltitude(0.0)
    , avgSpeed(0.0)
{
}

// MissionSimulator implementation

MissionSimulator::MissionSimulator(QObject *parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
    , m_simulationSpeed(1.0)
    , m_isRunning(false)
{
    connect(m_updateTimer, &QTimer::timeout, this, &MissionSimulator::updateSimulation);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
}

MissionSimulator::~MissionSimulator()
{
}

bool MissionSimulator::loadFlightPlan(const Models::FlightPlan& plan)
{
    if (plan.waypoints().isEmpty()) {
        qWarning() << "Cannot load empty flight plan";
        return false;
    }

    m_flightPlan = plan;
    reset();
    calculateStatistics();

    // Validate mission
    QStringList warnings = validateMission();
    for (const QString& warning : warnings) {
        emit this->warning(warning);
    }

    return true;
}

void MissionSimulator::start()
{
    if (m_flightPlan.waypoints().isEmpty()) {
        qWarning() << "No flight plan loaded";
        return;
    }

    m_isRunning = true;
    m_updateTimer->start();
}

void MissionSimulator::pause()
{
    m_isRunning = false;
    m_updateTimer->stop();
}

void MissionSimulator::stop()
{
    pause();
    reset();
}

void MissionSimulator::reset()
{
    m_currentState = SimulationState();

    if (!m_flightPlan.waypoints().isEmpty()) {
        m_currentState.currentPosition = m_flightPlan.waypoints().first().coordinate();
        m_currentState.currentAltitude = m_flightPlan.waypoints().first().coordinate().altitude();
    }

    emit stateUpdated(m_currentState);
}

void MissionSimulator::setSimulationSpeed(double speed)
{
    m_simulationSpeed = qMax(0.1, qMin(100.0, speed));
}

void MissionSimulator::jumpToWaypoint(int waypointIndex)
{
    if (waypointIndex < 0 || waypointIndex >= m_flightPlan.waypoints().size()) {
        return;
    }

    m_currentState.currentWaypointIndex = waypointIndex;
    m_currentState.currentPosition = m_flightPlan.waypoints()[waypointIndex].coordinate();
    m_currentState.currentAltitude = m_flightPlan.waypoints()[waypointIndex].coordinate().altitude();

    // Calculate accumulated distance and time
    double accumDist = 0.0;
    for (int i = 0; i < waypointIndex && i < m_flightPlan.waypoints().size() - 1; i++) {
        accumDist += Geospatial::GeoUtils::distanceBetween(
            m_flightPlan.waypoints()[i].coordinate(),
            m_flightPlan.waypoints()[i + 1].coordinate());
    }

    m_currentState.distanceTraveled = accumDist;
    m_currentState.elapsedTime = static_cast<int>(accumDist / DEFAULT_CRUISE_SPEED);
    m_currentState.currentBatteryPercent = 100.0 - calculateBatteryUsage(m_currentState.elapsedTime);
    m_currentState.photosTaken = estimatePhotoCount(m_currentState.elapsedTime);

    emit stateUpdated(m_currentState);
    emit waypointReached(waypointIndex);
}

double MissionSimulator::progress() const
{
    if (m_statistics.totalWaypoints <= 1) {
        return 0.0;
    }
    return static_cast<double>(m_currentState.currentWaypointIndex) / (m_statistics.totalWaypoints - 1);
}

QStringList MissionSimulator::validateMission() const
{
    return performSafetyChecks();
}

void MissionSimulator::updateSimulation()
{
    if (!m_isRunning || m_currentState.isComplete) {
        return;
    }

    const auto& waypoints = m_flightPlan.waypoints();
    if (waypoints.isEmpty() || m_currentState.currentWaypointIndex >= waypoints.size()) {
        m_currentState.isComplete = true;
        pause();
        emit simulationCompleted(m_statistics);
        return;
    }

    // Update time
    double deltaTime = (UPDATE_INTERVAL_MS / 1000.0) * m_simulationSpeed;
    m_currentState.elapsedTime += static_cast<int>(deltaTime);

    // Calculate segment progress
    if (m_currentState.currentWaypointIndex < waypoints.size() - 1) {
        const auto& currentWP = waypoints[m_currentState.currentWaypointIndex].coordinate();
        const auto& nextWP = waypoints[m_currentState.currentWaypointIndex + 1].coordinate();

        double segmentDist = Geospatial::GeoUtils::distanceBetween(currentWP, nextWP);
        double segmentTime = segmentDist / DEFAULT_CRUISE_SPEED;

        // Calculate time into current segment
        double segmentProgress = 0.0;
        if (segmentTime > 0) {
            double timeIntoSegment = fmod(m_currentState.elapsedTime, segmentTime);
            segmentProgress = timeIntoSegment / segmentTime;
        }

        // Interpolate position
        if (segmentProgress >= 1.0 || segmentDist < 0.1) {
            // Reached next waypoint
            m_currentState.currentWaypointIndex++;
            m_currentState.currentPosition = nextWP;
            m_currentState.currentAltitude = nextWP.altitude();
            emit waypointReached(m_currentState.currentWaypointIndex);

            // Check if complete
            if (m_currentState.currentWaypointIndex >= waypoints.size() - 1) {
                m_currentState.isComplete = true;
                pause();
                emit simulationCompleted(m_statistics);
                return;
            }
        } else {
            // Interpolate position between waypoints
            double lat = currentWP.latitude() + (nextWP.latitude() - currentWP.latitude()) * segmentProgress;
            double lon = currentWP.longitude() + (nextWP.longitude() - currentWP.longitude()) * segmentProgress;
            double alt = currentWP.altitude() + (nextWP.altitude() - currentWP.altitude()) * segmentProgress;

            m_currentState.currentPosition = Models::GeospatialCoordinate(lat, lon, alt);
            m_currentState.currentAltitude = alt;
        }

        m_currentState.currentSpeed = DEFAULT_CRUISE_SPEED;
        m_currentState.distanceTraveled += DEFAULT_CRUISE_SPEED * deltaTime;
    }

    // Update battery
    m_currentState.currentBatteryPercent = 100.0 - calculateBatteryUsage(m_currentState.elapsedTime);

    // Check for battery change
    if (m_currentState.currentBatteryPercent <= 20.0) {
        int batteryNum = static_cast<int>((100.0 - m_currentState.currentBatteryPercent) / 80.0) + 1;
        emit batteryChangeRequired(batteryNum);
        m_currentState.currentBatteryPercent = 100.0;  // Replace battery
    }

    // Update photos
    int expectedPhotos = estimatePhotoCount(m_currentState.elapsedTime);
    if (expectedPhotos > m_currentState.photosTaken) {
        m_currentState.photosTaken = expectedPhotos;
        emit photoCaptured(m_currentState.photosTaken, m_currentState.currentPosition);
    }

    emit stateUpdated(m_currentState);
}

void MissionSimulator::calculateStatistics()
{
    m_statistics = SimulationStatistics();

    const auto& waypoints = m_flightPlan.waypoints();
    if (waypoints.isEmpty()) {
        return;
    }

    m_statistics.totalWaypoints = waypoints.size();

    // Calculate total distance
    double totalDist = 0.0;
    double maxAlt = 0.0;

    for (int i = 0; i < waypoints.size() - 1; i++) {
        double segDist = Geospatial::GeoUtils::distanceBetween(
            waypoints[i].coordinate(),
            waypoints[i + 1].coordinate());
        totalDist += segDist;
        maxAlt = qMax(maxAlt, waypoints[i].coordinate().altitude());
    }
    if (!waypoints.isEmpty()) {
        maxAlt = qMax(maxAlt, waypoints.last().coordinate().altitude());
    }

    m_statistics.totalDistance = totalDist;
    m_statistics.maxAltitude = maxAlt;
    m_statistics.avgSpeed = DEFAULT_CRUISE_SPEED;

    // Calculate flight time
    m_statistics.totalFlightTime = static_cast<int>(totalDist / DEFAULT_CRUISE_SPEED);

    // Calculate photo count
    m_statistics.totalPhotoCount = estimatePhotoCount(m_statistics.totalFlightTime);

    // Calculate battery changes
    double totalBatteryUsage = calculateBatteryUsage(m_statistics.totalFlightTime);
    m_statistics.batteryChangesRequired = static_cast<int>(totalBatteryUsage / 100.0);

    // Calculate battery percentages at each waypoint
    m_statistics.batteryPercentages.clear();
    m_statistics.waypointTimes.clear();

    double accumTime = 0.0;
    QDateTime startTime = QDateTime::currentDateTime();

    for (int i = 0; i < waypoints.size(); i++) {
        if (i > 0) {
            double segDist = Geospatial::GeoUtils::distanceBetween(
                waypoints[i - 1].coordinate(),
                waypoints[i].coordinate());
            accumTime += segDist / DEFAULT_CRUISE_SPEED;
        }

        double batteryPercent = 100.0 - calculateBatteryUsage(accumTime);
        m_statistics.batteryPercentages.append(batteryPercent);
        m_statistics.waypointTimes.append(startTime.addSecs(static_cast<int>(accumTime)));
    }

    // Survey area (approximate)
    if (!m_flightPlan.surveyArea().isEmpty()) {
        // Use flight plan's survey area
        QPolygonF poly = m_flightPlan.surveyArea();

        // Simple area calculation for lat/lon polygon (approximate)
        double area = 0.0;
        for (int i = 0; i < poly.size(); i++) {
            int j = (i + 1) % poly.size();
            area += poly[i].x() * poly[j].y();
            area -= poly[j].x() * poly[i].y();
        }
        area = qAbs(area) / 2.0;

        // Convert from degrees^2 to meters^2 (very rough approximation)
        // At equator: 1 degree ≈ 111km
        area *= (111000.0 * 111000.0);

        m_statistics.surveyAreaCovered = area;
    }
}

double MissionSimulator::calculateBatteryUsage(double flightTime) const
{
    // Battery usage calculation
    // Capacity: 5000mAh, Flight current: 10A (10000mA)
    // Flight time on one battery: 5000mAh / 10000mA = 0.5h = 30 minutes

    double batteryLifeSeconds = (BATTERY_CAPACITY_MAH / FLIGHT_CURRENT_MA) * 3600.0;
    double percentageUsed = (flightTime / batteryLifeSeconds) * 100.0;

    return percentageUsed;
}

int MissionSimulator::estimatePhotoCount(double segmentTime) const
{
    // Photo every 2 seconds
    return static_cast<int>(segmentTime / PHOTO_INTERVAL);
}

QStringList MissionSimulator::performSafetyChecks() const
{
    QStringList warnings;

    const auto& waypoints = m_flightPlan.waypoints();

    if (waypoints.isEmpty()) {
        warnings << "Flight plan has no waypoints";
        return warnings;
    }

    // Check altitude limits
    for (int i = 0; i < waypoints.size(); i++) {
        double alt = waypoints[i].coordinate().altitude();
        if (alt > 120.0) {
            warnings << QString("Waypoint %1 exceeds 120m altitude limit (%.1f m)")
                .arg(i + 1).arg(alt);
        }
        if (alt < 5.0 && alt > 0.0) {
            warnings << QString("Waypoint %1 has very low altitude (%.1f m) - collision risk")
                .arg(i + 1).arg(alt);
        }
    }

    // Check total flight time
    if (m_statistics.totalFlightTime > 1800) {  // 30 minutes
        warnings << QString("Flight time (%.1f min) exceeds recommended single battery duration")
            .arg(m_statistics.totalFlightTime / 60.0);
    }

    // Check distance
    if (m_statistics.totalDistance > 10000.0) {  // 10km
        warnings << QString("Total distance (%.1f km) is very long - verify drone range")
            .arg(m_statistics.totalDistance / 1000.0);
    }

    // Check waypoint count
    if (waypoints.size() > 500) {
        warnings << "Flight plan has over 500 waypoints - may exceed drone memory";
    }

    // Check for rapid altitude changes
    for (int i = 1; i < waypoints.size(); i++) {
        double altChange = qAbs(waypoints[i].coordinate().altitude() - waypoints[i - 1].coordinate().altitude());
        double dist = Geospatial::GeoUtils::distanceBetween(
            waypoints[i - 1].coordinate(),
            waypoints[i].coordinate());

        if (altChange > 50.0 && dist < 100.0) {
            warnings << QString("Waypoints %1-%2: Rapid altitude change (%.1f m over %.1f m distance)")
                .arg(i).arg(i + 1).arg(altChange).arg(dist);
        }
    }

    // Check battery requirements
    if (m_statistics.batteryChangesRequired > 5) {
        warnings << QString("Mission requires %1 batteries - consider splitting into multiple missions")
            .arg(m_statistics.batteryChangesRequired + 1);
    }

    return warnings;
}

} // namespace Core
} // namespace DroneMapper
