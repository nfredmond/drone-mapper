#include "FlightPlan.h"
#include <QUuid>
#include <cmath>

namespace DroneMapper {
namespace Models {

FlightPlan::FlightPlan()
    : m_id(generateId())
    , m_name("New Flight Plan")
    , m_createdDate(QDateTime::currentDateTime())
    , m_modifiedDate(QDateTime::currentDateTime())
    , m_patternType(PatternType::Manual)
{
}

FlightPlan::FlightPlan(const QString& name)
    : m_id(generateId())
    , m_name(name)
    , m_createdDate(QDateTime::currentDateTime())
    , m_modifiedDate(QDateTime::currentDateTime())
    , m_patternType(PatternType::Manual)
{
}

void FlightPlan::addWaypoint(const Waypoint& waypoint)
{
    m_waypoints.append(waypoint);
    m_modifiedDate = QDateTime::currentDateTime();
}

void FlightPlan::removeWaypoint(int index)
{
    if (index >= 0 && index < m_waypoints.count()) {
        m_waypoints.removeAt(index);
        m_modifiedDate = QDateTime::currentDateTime();
    }
}

void FlightPlan::clearWaypoints()
{
    m_waypoints.clear();
    m_modifiedDate = QDateTime::currentDateTime();
}

double FlightPlan::totalDistance() const
{
    if (m_waypoints.count() < 2) {
        return 0.0;
    }

    double totalDist = 0.0;
    for (int i = 0; i < m_waypoints.count() - 1; ++i) {
        const auto& wp1 = m_waypoints[i].coordinate();
        const auto& wp2 = m_waypoints[i + 1].coordinate();

        // Haversine formula for distance calculation
        double lat1 = wp1.latitude() * M_PI / 180.0;
        double lat2 = wp2.latitude() * M_PI / 180.0;
        double dLat = (wp2.latitude() - wp1.latitude()) * M_PI / 180.0;
        double dLon = (wp2.longitude() - wp1.longitude()) * M_PI / 180.0;

        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(lat1) * std::cos(lat2) *
                   std::sin(dLon/2) * std::sin(dLon/2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
        double distance = 6371000 * c; // Earth radius in meters

        totalDist += distance;
    }

    return totalDist;
}

int FlightPlan::estimatedFlightTime() const
{
    double distance = totalDistance();
    double avgSpeed = m_parameters.flightSpeed();
    if (avgSpeed <= 0) avgSpeed = 8.0;

    int timeSeconds = static_cast<int>(distance / avgSpeed);
    int timeMinutes = (timeSeconds + 59) / 60; // Round up

    return timeMinutes;
}

int FlightPlan::estimatedPhotoCount() const
{
    double distance = totalDistance();
    double footprintHeight = m_parameters.imageFootprintHeight();
    double frontOverlap = m_parameters.frontOverlap() / 100.0;

    if (footprintHeight <= 0) return 0;

    double effectiveDistance = footprintHeight * (1.0 - frontOverlap);
    int photoCount = static_cast<int>(distance / effectiveDistance);

    return photoCount;
}

double FlightPlan::surveyAreaSize() const
{
    // Calculate polygon area using shoelace formula
    if (m_surveyArea.isEmpty()) {
        return 0.0;
    }

    double area = 0.0;
    int n = m_surveyArea.count();

    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += m_surveyArea[i].x() * m_surveyArea[j].y();
        area -= m_surveyArea[j].x() * m_surveyArea[i].y();
    }

    return std::abs(area) / 2.0;
}

bool FlightPlan::isValid() const
{
    if (m_name.isEmpty()) return false;
    if (m_waypoints.isEmpty()) return false;

    for (const auto& wp : m_waypoints) {
        if (!wp.coordinate().isValid()) return false;
    }

    return true;
}

QString FlightPlan::validationError() const
{
    if (m_name.isEmpty()) return "Flight plan name is empty";
    if (m_waypoints.isEmpty()) return "No waypoints defined";

    for (int i = 0; i < m_waypoints.count(); ++i) {
        if (!m_waypoints[i].coordinate().isValid()) {
            return QString("Invalid coordinate at waypoint %1").arg(i + 1);
        }
    }

    return QString();
}

QString FlightPlan::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace Models
} // namespace DroneMapper
