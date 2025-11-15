#include "GeoUtils.h"
#include <cmath>

namespace DroneMapper {
namespace Geospatial {

double GeoUtils::distanceBetween(
    const Models::GeospatialCoordinate& coord1,
    const Models::GeospatialCoordinate& coord2)
{
    // Haversine formula
    double lat1 = toRadians(coord1.latitude());
    double lat2 = toRadians(coord2.latitude());
    double dLat = toRadians(coord2.latitude() - coord1.latitude());
    double dLon = toRadians(coord2.longitude() - coord1.longitude());

    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dLon/2) * std::sin(dLon/2);

    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    double distance = EARTH_RADIUS * c;

    return distance;
}

double GeoUtils::bearingBetween(
    const Models::GeospatialCoordinate& coord1,
    const Models::GeospatialCoordinate& coord2)
{
    double lat1 = toRadians(coord1.latitude());
    double lat2 = toRadians(coord2.latitude());
    double dLon = toRadians(coord2.longitude() - coord1.longitude());

    double y = std::sin(dLon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) -
               std::sin(lat1) * std::cos(lat2) * std::cos(dLon);

    double bearing = std::atan2(y, x);
    bearing = toDegrees(bearing);
    bearing = normalizeAngle(bearing);

    return bearing;
}

Models::GeospatialCoordinate GeoUtils::destinationPoint(
    const Models::GeospatialCoordinate& origin,
    double distance,
    double bearing)
{
    double lat1 = toRadians(origin.latitude());
    double lon1 = toRadians(origin.longitude());
    double brng = toRadians(bearing);
    double d = distance / EARTH_RADIUS;

    double lat2 = std::asin(
        std::sin(lat1) * std::cos(d) +
        std::cos(lat1) * std::sin(d) * std::cos(brng)
    );

    double lon2 = lon1 + std::atan2(
        std::sin(brng) * std::sin(d) * std::cos(lat1),
        std::cos(d) - std::sin(lat1) * std::sin(lat2)
    );

    return Models::GeospatialCoordinate(
        toDegrees(lat2),
        toDegrees(lon2),
        origin.altitude()
    );
}

QPointF GeoUtils::toCartesian(
    const Models::GeospatialCoordinate& coord,
    const Models::GeospatialCoordinate& origin)
{
    double distance = distanceBetween(origin, coord);
    double bearing = bearingBetween(origin, coord);

    double x = distance * std::sin(toRadians(bearing));
    double y = distance * std::cos(toRadians(bearing));

    return QPointF(x, y);
}

Models::GeospatialCoordinate GeoUtils::fromCartesian(
    const QPointF& point,
    const Models::GeospatialCoordinate& origin)
{
    double distance = std::sqrt(point.x() * point.x() + point.y() * point.y());
    double bearing = toDegrees(std::atan2(point.x(), point.y()));
    bearing = normalizeAngle(bearing);

    return destinationPoint(origin, distance, bearing);
}

double GeoUtils::calculateArea(const QPolygonF& polygon)
{
    if (polygon.count() < 3) {
        return 0.0;
    }

    // Use spherical excess formula for accurate large-area calculation
    // For simplicity, using planar approximation here
    double area = 0.0;
    int n = polygon.count();

    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += polygon[i].x() * polygon[j].y();
        area -= polygon[j].x() * polygon[i].y();
    }

    return std::abs(area) / 2.0;
}

Models::GeospatialCoordinate GeoUtils::calculateCentroid(const QPolygonF& polygon)
{
    if (polygon.isEmpty()) {
        return Models::GeospatialCoordinate();
    }

    double sumLat = 0.0;
    double sumLon = 0.0;

    for (const QPointF& point : polygon) {
        sumLat += point.y();  // Assuming y = latitude
        sumLon += point.x();  // Assuming x = longitude
    }

    int count = polygon.count();
    return Models::GeospatialCoordinate(
        sumLat / count,
        sumLon / count,
        0.0
    );
}

double GeoUtils::normalizeAngle(double angle)
{
    while (angle < 0.0) {
        angle += 360.0;
    }
    while (angle >= 360.0) {
        angle -= 360.0;
    }
    return angle;
}

} // namespace Geospatial
} // namespace DroneMapper
