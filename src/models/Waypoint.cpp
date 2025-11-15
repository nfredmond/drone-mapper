#include "Waypoint.h"

namespace DroneMapper {
namespace Models {

Waypoint::Waypoint()
    : m_waypointNumber(0)
    , m_speed(8.0)
    , m_headingMode(HeadingMode::Auto)
    , m_heading(0.0)
    , m_hoverTime(0)
{
}

Waypoint::Waypoint(const GeospatialCoordinate& coordinate)
    : m_coordinate(coordinate)
    , m_waypointNumber(0)
    , m_speed(8.0)
    , m_headingMode(HeadingMode::Auto)
    , m_heading(0.0)
    , m_hoverTime(0)
{
}

} // namespace Models
} // namespace DroneMapper
