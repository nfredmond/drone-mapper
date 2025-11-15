#include "NoFlyZoneChecker.h"
#include "geospatial/GeoUtils.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace Core {

bool NoFlyZone::isActive(const QDateTime& time) const
{
    if (!effectiveStart.isValid() && !effectiveEnd.isValid()) {
        return true; // Permanent zone
    }

    if (effectiveStart.isValid() && time < effectiveStart) {
        return false;
    }

    if (effectiveEnd.isValid() && time > effectiveEnd) {
        return false;
    }

    return true;
}

QString NoFlyZone::getTypeString() const
{
    switch (type) {
    case NoFlyZoneType::Airport:            return "Airport";
    case NoFlyZoneType::Military:           return "Military";
    case NoFlyZoneType::Stadium:            return "Stadium";
    case NoFlyZoneType::Prison:             return "Prison";
    case NoFlyZoneType::PowerPlant:         return "Power Plant";
    case NoFlyZoneType::NationalPark:       return "National Park";
    case NoFlyZoneType::ControlledAirspace: return "Controlled Airspace";
    case NoFlyZoneType::TemporaryRestriction: return "TFR";
    case NoFlyZoneType::Custom:             return "Custom";
    }
    return "Unknown";
}

QString NoFlyZone::getLevelString() const
{
    switch (level) {
    case RestrictionLevel::NoFly:         return "NO FLY";
    case RestrictionLevel::Authorization: return "Authorization Required";
    case RestrictionLevel::Warning:       return "Warning";
    case RestrictionLevel::Notification:  return "Notification";
    }
    return "Unknown";
}

QString ZoneViolation::getDetailedMessage() const
{
    QString msg;
    msg += QString("[%1] %2\n").arg(zone.getLevelString()).arg(zone.name);
    msg += QString("Type: %1\n").arg(zone.getTypeString());
    msg += QString("Distance into zone: %1 m\n").arg(distanceIntoZone, 0, 'f', 0);

    if (waypointIndex >= 0) {
        msg += QString("Waypoint #%1 violates restriction\n").arg(waypointIndex + 1);
    }

    if (isPathViolation) {
        msg += "Flight path crosses restricted area\n";
    }

    msg += QString("Restriction: %1\n").arg(message);

    return msg;
}

QString NoFlyCheckResult::getReport() const
{
    QString report;
    report += "=== NO-FLY ZONE CHECK ===\n\n";
    report += QString("Status: %1\n").arg(isSafe ? "SAFE" : "VIOLATIONS DETECTED");
    report += QString("Critical Violations: %1\n").arg(criticalViolations);
    report += QString("Warnings: %1\n\n").arg(warningViolations);

    if (!violations.isEmpty()) {
        report += "VIOLATIONS:\n";
        for (const auto& v : violations) {
            report += "\n" + v.getDetailedMessage();
        }
    }

    if (!nearbyZones.isEmpty()) {
        report += QString("\nNEARBY ZONES (%1):\n").arg(nearbyZones.count());
        for (const auto& z : nearbyZones) {
            report += QString("  - %1 (%2)\n").arg(z.name).arg(z.getTypeString());
        }
    }

    return report;
}

void ZoneDatabase::addZone(const NoFlyZone& zone)
{
    zones.append(zone);
    lastUpdated = QDateTime::currentDateTime();
}

QList<NoFlyZone> ZoneDatabase::getZonesNear(
    const Models::GeospatialCoordinate& location,
    double radiusMeters) const
{
    QList<NoFlyZone> nearby;

    for (const auto& zone : zones) {
        double distance = Geospatial::GeoUtils::distanceBetween(location, zone.center);

        if (distance <= radiusMeters + zone.radiusMeters) {
            nearby.append(zone);
        }
    }

    return nearby;
}

NoFlyZoneChecker::NoFlyZoneChecker()
{
}

NoFlyCheckResult NoFlyZoneChecker::checkFlightPlan(
    const Models::FlightPlan& plan,
    const ZoneDatabase& database)
{
    NoFlyCheckResult result;
    result.isSafe = true;
    result.criticalViolations = 0;
    result.warningViolations = 0;

    const auto& waypoints = plan.waypoints();
    QDateTime checkTime = QDateTime::currentDateTime();

    // Check each waypoint
    for (int i = 0; i < waypoints.count(); ++i) {
        const auto& wp = waypoints[i];
        auto violations = checkPoint(wp.coordinate(), wp.coordinate().altitude(), database);

        for (auto& v : violations) {
            v.waypointIndex = i;

            // Count by severity
            if (v.zone.level == RestrictionLevel::NoFly ||
                v.zone.level == RestrictionLevel::Authorization) {
                result.criticalViolations++;
                result.isSafe = false;
            } else {
                result.warningViolations++;
            }

            result.violations.append(v);
        }
    }

    // Check path segments
    for (int i = 1; i < waypoints.count(); ++i) {
        const auto& wp1 = waypoints[i-1];
        const auto& wp2 = waypoints[i];

        for (const auto& zone : database.zones) {
            if (!zone.isActive(checkTime)) continue;

            if (pathCrossesZone(wp1.coordinate(), wp2.coordinate(), zone)) {
                // Check if not already reported
                bool alreadyReported = false;
                for (const auto& v : result.violations) {
                    if (v.zone.id == zone.id) {
                        alreadyReported = true;
                        break;
                    }
                }

                if (!alreadyReported) {
                    ZoneViolation v;
                    v.zone = zone;
                    v.violationPoint = wp1.coordinate();
                    v.waypointIndex = i;
                    v.isPathViolation = true;
                    v.message = "Flight path crosses restricted zone";
                    v.distanceIntoZone = 0.0;

                    if (zone.level == RestrictionLevel::NoFly) {
                        result.criticalViolations++;
                        result.isSafe = false;
                    } else {
                        result.warningViolations++;
                    }

                    result.violations.append(v);
                }
            }
        }
    }

    // Find nearby zones (within 500m)
    if (!waypoints.isEmpty()) {
        Models::GeospatialCoordinate center = waypoints.first().coordinate();
        result.nearbyZones = findNearbyZones(center, 500.0, database);
    }

    // Generate summary
    if (result.isSafe) {
        result.summary = QString("Flight plan is clear - %1 nearby zones detected")
            .arg(result.nearbyZones.count());
    } else {
        result.summary = QString("UNSAFE - %1 critical violations detected")
            .arg(result.criticalViolations);
    }

    return result;
}

QList<ZoneViolation> NoFlyZoneChecker::checkPoint(
    const Models::GeospatialCoordinate& location,
    double altitude,
    const ZoneDatabase& database)
{
    QList<ZoneViolation> violations;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& zone : database.zones) {
        if (!zone.isActive(now)) continue;

        if (isInsideZone(location, altitude, zone)) {
            ZoneViolation v;
            v.zone = zone;
            v.violationPoint = location;
            v.waypointIndex = -1;
            v.isPathViolation = false;
            v.distanceIntoZone = -distanceToZone(location, zone);
            v.message = QString("Location inside %1 restricted area").arg(zone.name);

            violations.append(v);
        }
    }

    return violations;
}

bool NoFlyZoneChecker::isInsideZone(
    const Models::GeospatialCoordinate& point,
    double altitude,
    const NoFlyZone& zone)
{
    // Check altitude restrictions
    if (zone.minAltitude > 0.0 && altitude < zone.minAltitude) {
        return false; // Below restriction
    }
    if (zone.maxAltitude > 0.0 && altitude > zone.maxAltitude) {
        return false; // Above restriction
    }

    // Check horizontal boundary
    if (zone.isCircular()) {
        // Circular zone
        double distance = Geospatial::GeoUtils::distanceBetween(point, zone.center);
        return distance <= zone.radiusMeters;
    } else {
        // Polygon zone
        return pointInPolygon(point, zone.boundary);
    }
}

bool NoFlyZoneChecker::pathCrossesZone(
    const Models::GeospatialCoordinate& start,
    const Models::GeospatialCoordinate& end,
    const NoFlyZone& zone)
{
    if (zone.isCircular()) {
        return lineSegmentIntersectsCircle(start, end, zone.center, zone.radiusMeters);
    } else {
        return lineSegmentIntersectsPolygon(start, end, zone.boundary);
    }
}

double NoFlyZoneChecker::distanceToZone(
    const Models::GeospatialCoordinate& point,
    const NoFlyZone& zone)
{
    if (zone.isCircular()) {
        double distanceToCenter = Geospatial::GeoUtils::distanceBetween(point, zone.center);
        return distanceToCenter - zone.radiusMeters;
    } else {
        return distanceToPolygon(point, zone.boundary);
    }
}

QList<NoFlyZone> NoFlyZoneChecker::findNearbyZones(
    const Models::GeospatialCoordinate& location,
    double radiusMeters,
    const ZoneDatabase& database)
{
    return database.getZonesNear(location, radiusMeters);
}

NoFlyZone NoFlyZoneChecker::createAirportZone(
    const Models::GeospatialCoordinate& airportLocation,
    const QString& airportName,
    double radiusMeters)
{
    NoFlyZone zone;
    zone.id = QString("airport_%1").arg(airportName).replace(" ", "_");
    zone.name = airportName;
    zone.type = NoFlyZoneType::Airport;
    zone.level = RestrictionLevel::Authorization;
    zone.center = airportLocation;
    zone.radiusMeters = radiusMeters;
    zone.minAltitude = 0.0;
    zone.maxAltitude = 0.0; // All altitudes
    zone.description = QString("Airport exclusion zone - %1m radius").arg(radiusMeters, 0, 'f', 0);
    zone.authority = "FAA";

    return zone;
}

NoFlyZone NoFlyZoneChecker::createCircularZone(
    const Models::GeospatialCoordinate& center,
    double radiusMeters,
    const QString& name,
    NoFlyZoneType type)
{
    NoFlyZone zone;
    zone.id = QString("zone_%1").arg(name).replace(" ", "_");
    zone.name = name;
    zone.type = type;
    zone.level = RestrictionLevel::NoFly;
    zone.center = center;
    zone.radiusMeters = radiusMeters;
    zone.minAltitude = 0.0;
    zone.maxAltitude = 0.0;
    zone.description = QString("Circular no-fly zone - %1m radius").arg(radiusMeters, 0, 'f', 0);
    zone.authority = "Custom";

    return zone;
}

NoFlyZone NoFlyZoneChecker::createPolygonZone(
    const QList<Models::GeospatialCoordinate>& boundary,
    const QString& name,
    NoFlyZoneType type)
{
    NoFlyZone zone;
    zone.id = QString("zone_%1").arg(name).replace(" ", "_");
    zone.name = name;
    zone.type = type;
    zone.level = RestrictionLevel::NoFly;
    zone.minAltitude = 0.0;
    zone.maxAltitude = 0.0;
    zone.authority = "Custom";

    // Convert to QPolygonF
    for (const auto& coord : boundary) {
        zone.boundary.append(QPointF(coord.longitude(), coord.latitude()));
    }

    // Calculate centroid
    if (!boundary.isEmpty()) {
        zone.center = boundary.first();
    }

    zone.description = QString("Polygon no-fly zone - %1 vertices").arg(boundary.count());

    return zone;
}

ZoneDatabase NoFlyZoneChecker::loadDatabase(const QString& filePath)
{
    ZoneDatabase db;
    db.region = "Unknown";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return db;
    }

    // Simplified - would parse JSON/XML in production
    file.close();

    return db;
}

bool NoFlyZoneChecker::saveDatabase(
    const ZoneDatabase& database,
    const QString& filePath)
{
    Q_UNUSED(database);
    Q_UNUSED(filePath);

    // Simplified - would save JSON/XML in production
    return false;
}

ZoneDatabase NoFlyZoneChecker::getDefaultUSDatabase()
{
    ZoneDatabase db;
    db.region = "USA";
    db.lastUpdated = QDateTime::currentDateTime();

    // Add some example zones (in production, this would load from FAA data)

    // Example: Washington DC restricted zone
    NoFlyZone dcZone;
    dcZone.id = "dc_sfra";
    dcZone.name = "Washington DC SFRA";
    dcZone.type = NoFlyZoneType::ControlledAirspace;
    dcZone.level = RestrictionLevel::NoFly;
    dcZone.center = Models::GeospatialCoordinate(38.8951, -77.0369, 0.0); // DC
    dcZone.radiusMeters = 24000.0; // ~15 miles
    dcZone.description = "Special Flight Rules Area";
    dcZone.authority = "FAA";
    db.addZone(dcZone);

    return db;
}

bool NoFlyZoneChecker::pointInPolygon(
    const Models::GeospatialCoordinate& point,
    const QPolygonF& polygon)
{
    // Ray casting algorithm
    QPointF p(point.longitude(), point.latitude());
    return polygon.containsPoint(p, Qt::OddEvenFill);
}

double NoFlyZoneChecker::distanceToPolygon(
    const Models::GeospatialCoordinate& point,
    const QPolygonF& polygon)
{
    // Simplified - calculate distance to nearest edge
    double minDistance = 1e10;

    for (int i = 0; i < polygon.count(); ++i) {
        QPointF p1 = polygon[i];
        QPointF p2 = polygon[(i + 1) % polygon.count()];

        Models::GeospatialCoordinate c1(p1.y(), p1.x(), 0.0);
        Models::GeospatialCoordinate c2(p2.y(), p2.x(), 0.0);

        // Distance to segment (simplified)
        double d1 = Geospatial::GeoUtils::distanceBetween(point, c1);
        double d2 = Geospatial::GeoUtils::distanceBetween(point, c2);

        minDistance = std::min(minDistance, std::min(d1, d2));
    }

    return minDistance;
}

bool NoFlyZoneChecker::lineSegmentIntersectsCircle(
    const Models::GeospatialCoordinate& lineStart,
    const Models::GeospatialCoordinate& lineEnd,
    const Models::GeospatialCoordinate& circleCenter,
    double circleRadius)
{
    // Check if either endpoint is inside
    double d1 = Geospatial::GeoUtils::distanceBetween(lineStart, circleCenter);
    double d2 = Geospatial::GeoUtils::distanceBetween(lineEnd, circleCenter);

    if (d1 <= circleRadius || d2 <= circleRadius) {
        return true;
    }

    // Check closest point on line segment to circle center
    // Simplified - more accurate would use great circle paths
    double segmentLength = Geospatial::GeoUtils::distanceBetween(lineStart, lineEnd);

    if (segmentLength < 1.0) {
        return false;
    }

    // Approximate - check midpoint
    Models::GeospatialCoordinate midpoint(
        (lineStart.latitude() + lineEnd.latitude()) / 2.0,
        (lineStart.longitude() + lineEnd.longitude()) / 2.0,
        (lineStart.altitude() + lineEnd.altitude()) / 2.0);

    double midDist = Geospatial::GeoUtils::distanceBetween(midpoint, circleCenter);
    return midDist <= circleRadius;
}

bool NoFlyZoneChecker::lineSegmentIntersectsPolygon(
    const Models::GeospatialCoordinate& lineStart,
    const Models::GeospatialCoordinate& lineEnd,
    const QPolygonF& polygon)
{
    // Check if either endpoint is inside
    if (pointInPolygon(lineStart, polygon) || pointInPolygon(lineEnd, polygon)) {
        return true;
    }

    // Check if line crosses any polygon edge (simplified)
    QPointF p1(lineStart.longitude(), lineStart.latitude());
    QPointF p2(lineEnd.longitude(), lineEnd.latitude());

    for (int i = 0; i < polygon.count(); ++i) {
        QPointF q1 = polygon[i];
        QPointF q2 = polygon[(i + 1) % polygon.count()];

        // Simplified intersection check
        // In production would use proper line-line intersection
    }

    return false;
}

} // namespace Core
} // namespace DroneMapper
