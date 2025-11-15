#ifndef FLIGHTPLAN_H
#define FLIGHTPLAN_H

#include "Waypoint.h"
#include "MissionParameters.h"
#include "GeospatialCoordinate.h"
#include <QString>
#include <QList>
#include <QtGui/QPolygonF>
#include <QDateTime>

namespace DroneMapper {
namespace Models {

/**
 * @brief Represents a complete flight plan with waypoints and parameters
 */
class FlightPlan {
public:
    enum class PatternType {
        Manual,           // User-defined waypoints
        Polygon,          // Coverage of polygon area
        Grid,             // Grid pattern
        Circular,         // Circular pattern around POI
        Corridor          // Linear corridor mapping
    };

    FlightPlan();
    FlightPlan(const QString& name);

    // Basic properties
    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    QDateTime createdDate() const { return m_createdDate; }
    QDateTime modifiedDate() const { return m_modifiedDate; }

    void setName(const QString& name) { m_name = name; }
    void setDescription(const QString& desc) { m_description = desc; }

    // Waypoints
    QList<Waypoint> waypoints() const { return m_waypoints; }
    void addWaypoint(const Waypoint& waypoint);
    void removeWaypoint(int index);
    void clearWaypoints();
    int waypointCount() const { return m_waypoints.count(); }

    // Mission parameters
    MissionParameters& parameters() { return m_parameters; }
    const MissionParameters& parameters() const { return m_parameters; }

    // Pattern definition
    PatternType patternType() const { return m_patternType; }
    void setPatternType(PatternType type) { m_patternType = type; }
    QPolygonF surveyArea() const { return m_surveyArea; }
    void setSurveyArea(const QPolygonF& area) { m_surveyArea = area; }

    // Calculations
    double totalDistance() const;        // Total flight distance in meters
    int estimatedFlightTime() const;     // Minutes
    int estimatedPhotoCount() const;
    double surveyAreaSize() const;       // Square meters

    // Validation
    bool isValid() const;
    QString validationError() const;

private:
    QString m_id;
    QString m_name;
    QString m_description;
    QDateTime m_createdDate;
    QDateTime m_modifiedDate;

    QList<Waypoint> m_waypoints;
    MissionParameters m_parameters;
    PatternType m_patternType;
    QPolygonF m_surveyArea;

    QString generateId() const;
};

} // namespace Models
} // namespace DroneMapper

#endif // FLIGHTPLAN_H
