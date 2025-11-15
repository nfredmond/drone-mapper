#ifndef WAYPOINT_H
#define WAYPOINT_H

#include "GeospatialCoordinate.h"
#include <QString>
#include <QList>

namespace DroneMapper {
namespace Models {

/**
 * @brief Represents a single waypoint in a flight plan
 */
class Waypoint {
public:
    enum class Action {
        None,
        TakePhoto,
        StartVideo,
        StopVideo,
        Hover,
        RotateAircraft
    };

    enum class HeadingMode {
        Auto,               // Follow flight path
        Fixed,              // Maintain specific heading
        PointOfInterest,    // Face POI
        Manual              // Custom heading per waypoint
    };

    Waypoint();
    Waypoint(const GeospatialCoordinate& coordinate);

    // Getters
    GeospatialCoordinate coordinate() const { return m_coordinate; }
    int waypointNumber() const { return m_waypointNumber; }
    double speed() const { return m_speed; }
    HeadingMode headingMode() const { return m_headingMode; }
    double heading() const { return m_heading; }
    QList<Action> actions() const { return m_actions; }
    int hoverTime() const { return m_hoverTime; }

    // Setters
    void setCoordinate(const GeospatialCoordinate& coord) { m_coordinate = coord; }
    void setWaypointNumber(int number) { m_waypointNumber = number; }
    void setSpeed(double speed) { m_speed = speed; }
    void setHeadingMode(HeadingMode mode) { m_headingMode = mode; }
    void setHeading(double heading) { m_heading = heading; }
    void addAction(Action action) { m_actions.append(action); }
    void setHoverTime(int seconds) { m_hoverTime = seconds; }

private:
    GeospatialCoordinate m_coordinate;
    int m_waypointNumber;
    double m_speed;              // m/s
    HeadingMode m_headingMode;
    double m_heading;            // Degrees (0-360)
    QList<Action> m_actions;
    int m_hoverTime;             // Seconds
};

} // namespace Models
} // namespace DroneMapper

#endif // WAYPOINT_H
