#ifndef COVERAGEPATTERNGENERATOR_H
#define COVERAGEPATTERNGENERATOR_H

#include "FlightPlan.h"
#include "GeospatialCoordinate.h"
#include <QtGui/QPolygonF>
#include <QList>

namespace DroneMapper {
namespace Geospatial {

/**
 * @brief Generates flight path coverage patterns for survey areas
 */
class CoveragePatternGenerator {
public:
    enum class Pattern {
        Parallel,      // Parallel lines (lawnmower)
        Grid,          // Perpendicular grid
        Circular,      // Circular around center
        Spiral         // Spiral from center
    };

    CoveragePatternGenerator();

    /**
     * @brief Generate parallel line pattern for polygon coverage
     * @param polygon Survey area polygon (lat/lon coordinates)
     * @param altitude Flight altitude in meters
     * @param direction Flight line direction in degrees (0-360)
     * @param spacing Distance between flight lines in meters
     * @param overlap Side overlap percentage (0-100)
     * @return List of waypoints covering the area
     */
    QList<Models::Waypoint> generateParallelLines(
        const QPolygonF& polygon,
        double altitude,
        double direction,
        double spacing,
        double overlap);

    /**
     * @brief Generate grid pattern (perpendicular passes)
     * @param polygon Survey area
     * @param altitude Flight altitude
     * @param spacing Line spacing
     * @return Waypoints for grid pattern
     */
    QList<Models::Waypoint> generateGrid(
        const QPolygonF& polygon,
        double altitude,
        double spacing);

    /**
     * @brief Generate circular pattern around point of interest
     * @param center Center point (POI)
     * @param radius Radius in meters
     * @param altitude Flight altitude
     * @param points Number of waypoints in circle
     * @return Waypoints for circular pattern
     */
    QList<Models::Waypoint> generateCircular(
        const Models::GeospatialCoordinate& center,
        double radius,
        double altitude,
        int points);

    /**
     * @brief Calculate optimal spacing based on camera parameters
     * @param altitude Flight altitude
     * @param sideOverlap Desired overlap percentage
     * @param sensorWidth Sensor width in mm
     * @param focalLength Focal length in mm
     * @return Recommended spacing in meters
     */
    double calculateOptimalSpacing(
        double altitude,
        double sideOverlap,
        double sensorWidth,
        double focalLength);

    QString lastError() const { return m_lastError; }

private:
    QList<QPointF> generateParallelLinePoints(
        const QPolygonF& polygon,
        double direction,
        double spacing);

    bool pointInPolygon(const QPointF& point, const QPolygonF& polygon);
    QPolygonF rotatePolygon(const QPolygonF& polygon, double angleDegrees);
    QPolygonF translatePolygon(const QPolygonF& polygon, const QPointF& offset);

    QString m_lastError;
};

} // namespace Geospatial
} // namespace DroneMapper

#endif // COVERAGEPATTERNGENERATOR_H
