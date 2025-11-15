#ifndef GEOUTILS_H
#define GEOUTILS_H

#include "GeospatialCoordinate.h"
#include <QtCore/QPointF>
#include <QtGui/QPolygonF>

namespace DroneMapper {
namespace Geospatial {

/**
 * @brief Utility functions for geospatial calculations
 */
class GeoUtils {
public:
    /**
     * @brief Calculate distance between two coordinates (Haversine formula)
     * @param coord1 First coordinate
     * @param coord2 Second coordinate
     * @return Distance in meters
     */
    static double distanceBetween(
        const Models::GeospatialCoordinate& coord1,
        const Models::GeospatialCoordinate& coord2);

    /**
     * @brief Calculate bearing from coord1 to coord2
     * @param coord1 Starting coordinate
     * @param coord2 Ending coordinate
     * @return Bearing in degrees (0-360)
     */
    static double bearingBetween(
        const Models::GeospatialCoordinate& coord1,
        const Models::GeospatialCoordinate& coord2);

    /**
     * @brief Calculate destination point given distance and bearing
     * @param origin Starting coordinate
     * @param distance Distance in meters
     * @param bearing Bearing in degrees
     * @return Destination coordinate
     */
    static Models::GeospatialCoordinate destinationPoint(
        const Models::GeospatialCoordinate& origin,
        double distance,
        double bearing);

    /**
     * @brief Convert lat/lon to local Cartesian coordinates (meters)
     * @param coord Geospatial coordinate
     * @param origin Reference origin point
     * @return QPointF with x,y in meters from origin
     */
    static QPointF toCartesian(
        const Models::GeospatialCoordinate& coord,
        const Models::GeospatialCoordinate& origin);

    /**
     * @brief Convert local Cartesian to lat/lon
     * @param point Point in meters (x, y)
     * @param origin Reference origin
     * @return Geospatial coordinate
     */
    static Models::GeospatialCoordinate fromCartesian(
        const QPointF& point,
        const Models::GeospatialCoordinate& origin);

    /**
     * @brief Calculate polygon area in square meters
     * @param polygon Polygon with lat/lon coordinates
     * @return Area in square meters
     */
    static double calculateArea(const QPolygonF& polygon);

    /**
     * @brief Calculate polygon centroid
     * @param polygon Polygon with lat/lon coordinates
     * @return Centroid coordinate
     */
    static Models::GeospatialCoordinate calculateCentroid(const QPolygonF& polygon);

    /**
     * @brief Normalize angle to 0-360 range
     * @param angle Angle in degrees
     * @return Normalized angle
     */
    static double normalizeAngle(double angle);

    /**
     * @brief Convert degrees to radians
     */
    static constexpr double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    /**
     * @brief Convert radians to degrees
     */
    static constexpr double toDegrees(double radians) {
        return radians * 180.0 / M_PI;
    }

    /**
     * @brief Earth radius in meters (WGS84 mean radius)
     */
    static constexpr double EARTH_RADIUS = 6371000.0;

private:
    GeoUtils() = delete;  // Static class, no instantiation
};

} // namespace Geospatial
} // namespace DroneMapper

#endif // GEOUTILS_H
