#ifndef NOFLYZONECHECK_H
#define NOFLYZONECHECK_H

#include "FlightPlan.h"
#include "GeospatialCoordinate.h"
#include <QString>
#include <QList>
#include <QPolygonF>
#include <QRectF>

namespace DroneMapper {
namespace Core {

/**
 * @brief Types of no-fly zones
 */
enum class NoFlyZoneType {
    Airport,            // Airport/heliport
    Military,           // Military installation
    Stadium,            // Sports stadium
    Prison,             // Correctional facility
    PowerPlant,         // Power plant/nuclear facility
    NationalPark,       // National park/protected area
    ControlledAirspace, // Class B/C/D airspace
    TemporaryRestriction, // TFR (Temporary Flight Restriction)
    Custom              // User-defined zone
};

/**
 * @brief Restriction severity
 */
enum class RestrictionLevel {
    NoFly,              // Absolute prohibition
    Authorization,      // Requires authorization
    Warning,            // Caution advised
    Notification        // Notification recommended
};

/**
 * @brief No-fly zone definition
 */
struct NoFlyZone {
    QString id;
    QString name;
    NoFlyZoneType type;
    RestrictionLevel level;
    Models::GeospatialCoordinate center;
    double radiusMeters;            // For circular zones
    QPolygonF boundary;             // For polygon zones (lat/lon)
    double minAltitude;             // Restriction starts (meters MSL)
    double maxAltitude;             // Restriction ends (meters MSL)
    QString description;
    QString authority;              // FAA, local, etc.
    QDateTime effectiveStart;
    QDateTime effectiveEnd;

    bool isCircular() const { return boundary.isEmpty(); }
    bool isActive(const QDateTime& time) const;
    QString getTypeString() const;
    QString getLevelString() const;
};

/**
 * @brief Zone violation
 */
struct ZoneViolation {
    NoFlyZone zone;
    Models::GeospatialCoordinate violationPoint;
    double distanceIntoZone;        // Meters inside zone
    int waypointIndex;              // Which waypoint violates
    QString message;
    bool isPathViolation;           // True if flight path crosses

    QString getDetailedMessage() const;
};

/**
 * @brief Check result
 */
struct NoFlyCheckResult {
    bool isSafe;                    // Overall safety
    QList<ZoneViolation> violations;
    QList<NoFlyZone> nearbyZones;   // Zones within 500m
    int criticalViolations;
    int warningViolations;
    QString summary;

    QString getReport() const;
};

/**
 * @brief No-fly zone database entry
 */
struct ZoneDatabase {
    QList<NoFlyZone> zones;
    QString region;                 // "USA", "EU", etc.
    QDateTime lastUpdated;

    void addZone(const NoFlyZone& zone);
    QList<NoFlyZone> getZonesNear(
        const Models::GeospatialCoordinate& location,
        double radiusMeters) const;
};

/**
 * @brief Checks flight plans against no-fly zones
 *
 * Features:
 * - Airport proximity checking (5km/3mi default)
 * - Controlled airspace detection
 * - TFR (Temporary Flight Restriction) awareness
 * - Custom zone support
 * - Path intersection detection
 * - Altitude-specific restrictions
 * - Time-based restrictions
 * - Multiple zone database support
 *
 * Data sources (production):
 * - FAA NOTAM/TFR data
 * - Airmap/B4UFLY integration
 * - OpenAIP database
 * - Custom user zones
 */
class NoFlyZoneChecker {
public:
    NoFlyZoneChecker();

    /**
     * @brief Check flight plan for zone violations
     * @param plan Flight plan to check
     * @param database Zone database
     * @return Check result with violations
     */
    static NoFlyCheckResult checkFlightPlan(
        const Models::FlightPlan& plan,
        const ZoneDatabase& database);

    /**
     * @brief Check single point against zones
     * @param location Location to check
     * @param altitude Altitude in meters MSL
     * @param database Zone database
     * @return List of violations
     */
    static QList<ZoneViolation> checkPoint(
        const Models::GeospatialCoordinate& location,
        double altitude,
        const ZoneDatabase& database);

    /**
     * @brief Check if point is inside zone
     * @param point Location to check
     * @param altitude Altitude MSL
     * @param zone Zone to check against
     * @return True if inside zone
     */
    static bool isInsideZone(
        const Models::GeospatialCoordinate& point,
        double altitude,
        const NoFlyZone& zone);

    /**
     * @brief Check if path crosses zone
     * @param start Path start point
     * @param end Path end point
     * @param zone Zone to check
     * @return True if path intersects zone
     */
    static bool pathCrossesZone(
        const Models::GeospatialCoordinate& start,
        const Models::GeospatialCoordinate& end,
        const NoFlyZone& zone);

    /**
     * @brief Calculate distance to zone boundary
     * @param point Location
     * @param zone Zone
     * @return Distance in meters (negative if inside)
     */
    static double distanceToZone(
        const Models::GeospatialCoordinate& point,
        const NoFlyZone& zone);

    /**
     * @brief Find zones near location
     * @param location Center point
     * @param radiusMeters Search radius
     * @param database Zone database
     * @return List of nearby zones
     */
    static QList<NoFlyZone> findNearbyZones(
        const Models::GeospatialCoordinate& location,
        double radiusMeters,
        const ZoneDatabase& database);

    /**
     * @brief Create airport exclusion zone
     * @param airportLocation Airport coordinates
     * @param airportName Airport name
     * @param radiusMeters Exclusion radius (default 5000m)
     * @return No-fly zone
     */
    static NoFlyZone createAirportZone(
        const Models::GeospatialCoordinate& airportLocation,
        const QString& airportName,
        double radiusMeters = 5000.0);

    /**
     * @brief Create circular no-fly zone
     * @param center Center point
     * @param radiusMeters Radius
     * @param name Zone name
     * @param type Zone type
     * @return No-fly zone
     */
    static NoFlyZone createCircularZone(
        const Models::GeospatialCoordinate& center,
        double radiusMeters,
        const QString& name,
        NoFlyZoneType type = NoFlyZoneType::Custom);

    /**
     * @brief Create polygon no-fly zone
     * @param boundary Polygon vertices (lat/lon)
     * @param name Zone name
     * @param type Zone type
     * @return No-fly zone
     */
    static NoFlyZone createPolygonZone(
        const QList<Models::GeospatialCoordinate>& boundary,
        const QString& name,
        NoFlyZoneType type = NoFlyZoneType::Custom);

    /**
     * @brief Load zone database from file
     * @param filePath Database file path
     * @return Zone database
     */
    static ZoneDatabase loadDatabase(const QString& filePath);

    /**
     * @brief Save zone database to file
     * @param database Database to save
     * @param filePath Output file path
     * @return True on success
     */
    static bool saveDatabase(
        const ZoneDatabase& database,
        const QString& filePath);

    /**
     * @brief Get default US zone database
     * @return Database with common US restrictions
     */
    static ZoneDatabase getDefaultUSDatabase();

private:
    static bool pointInPolygon(
        const Models::GeospatialCoordinate& point,
        const QPolygonF& polygon);

    static double distanceToPolygon(
        const Models::GeospatialCoordinate& point,
        const QPolygonF& polygon);

    static bool lineSegmentIntersectsCircle(
        const Models::GeospatialCoordinate& lineStart,
        const Models::GeospatialCoordinate& lineEnd,
        const Models::GeospatialCoordinate& circleCenter,
        double circleRadius);

    static bool lineSegmentIntersectsPolygon(
        const Models::GeospatialCoordinate& lineStart,
        const Models::GeospatialCoordinate& lineEnd,
        const QPolygonF& polygon);
};

} // namespace Core
} // namespace DroneMapper

#endif // NOFLYZONECHECK_H
