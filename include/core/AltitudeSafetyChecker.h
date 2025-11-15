#ifndef ALTITUDESAFETYCHECKER_H
#define ALTITUDESAFETYCHECKER_H

#include "FlightPlan.h"
#include "MissionParameters.h"
#include "GeospatialCoordinate.h"
#include <QList>
#include <QString>

namespace DroneMapper {
namespace Core {

/**
 * @brief Altitude violation severity levels
 */
enum class ViolationSeverity {
    Safe,           // No issues
    Warning,        // Advisory only
    Caution,        // Proceed with care
    Critical,       // Unsafe - do not fly
    Illegal         // Violates regulations
};

/**
 * @brief Altitude safety violation
 */
struct AltitudeViolation {
    ViolationSeverity severity;
    QString type;                           // "Regulatory", "Terrain", "Obstacle", "Capability"
    QString description;                    // Human-readable explanation
    Models::GeospatialCoordinate location;  // Where violation occurs
    double altitude;                        // Altitude at violation (meters)
    double limit;                           // Limit that was exceeded (meters)
    double exceedance;                      // How much over limit (meters)
    int waypointIndex;                      // Which waypoint has issue (-1 if N/A)
    QString recommendation;                 // Suggested action

    QString getSeverityText() const;
    QColor getSeverityColor() const;
};

/**
 * @brief Safety check results
 */
struct SafetyCheckResult {
    bool isSafe;                                // Overall safety status
    QList<AltitudeViolation> violations;        // All violations found
    int criticalCount;                          // Number of critical violations
    int cautionCount;                           // Number of caution violations
    int warningCount;                           // Number of warnings
    double maxAltitude;                         // Highest point in plan (meters AGL)
    double minGroundClearance;                  // Minimum clearance above terrain (meters)
    double averageAltitude;                     // Average flight altitude (meters AGL)
    QString summary;                            // Human-readable summary

    QString getDetailedReport() const;
};

/**
 * @brief Regulatory limits for different regions
 */
struct RegulatoryLimits {
    QString region;                    // "USA (FAA)", "EU (EASA)", "Canada", etc.
    double maxAltitudeAGL;            // Maximum altitude above ground (meters)
    double maxAltitudeMSL;            // Maximum absolute altitude (meters)
    double minGroundClearance;        // Minimum terrain clearance (meters)
    double minObstacleClearance;      // Minimum obstacle clearance (meters)
    bool requiresVisualLineOfSight;   // VLOS requirement
    double maxDistanceFromOperator;   // Maximum range (meters)
    QString notes;                     // Additional restrictions

    static RegulatoryLimits getDefaults();
    static RegulatoryLimits getFAA();      // USA
    static RegulatoryLimits getEASA();     // Europe
    static RegulatoryLimits getTransportCanada();
};

/**
 * @brief Drone capability limits
 */
struct DroneCapabilities {
    double maxAltitude;              // Maximum rated altitude (meters AGL)
    double maxFlightCeiling;         // Absolute ceiling (meters MSL)
    double minSafeAltitude;          // Minimum safe operating altitude (meters)
    double recommendedAltitudeRange; // Optimal operating range (meters)

    static DroneCapabilities getCapabilities(
        Models::MissionParameters::CameraModel model);
};

/**
 * @brief Checks flight plan altitude safety
 *
 * Features:
 * - Regulatory compliance checking (FAA Part 107, EASA, etc.)
 * - Terrain clearance validation
 * - Obstacle detection and clearance
 * - Drone capability verification
 * - Multi-waypoint analysis
 * - Automatic violation detection
 * - Safety recommendations
 * - Detailed reporting
 *
 * Checks performed:
 * - Maximum altitude limits (AGL and MSL)
 * - Minimum ground clearance
 * - Terrain following validation
 * - Obstacle proximity
 * - Regulatory compliance
 * - Drone operational limits
 */
class AltitudeSafetyChecker {
public:
    AltitudeSafetyChecker();

    /**
     * @brief Perform comprehensive safety check on flight plan
     * @param plan Flight plan to check
     * @param limits Regulatory limits to enforce
     * @return Safety check results with all violations
     */
    static SafetyCheckResult checkFlightPlan(
        const Models::FlightPlan& plan,
        const RegulatoryLimits& limits = RegulatoryLimits::getDefaults());

    /**
     * @brief Check single waypoint for safety
     * @param waypoint Waypoint to check
     * @param limits Regulatory limits
     * @return List of violations at this waypoint
     */
    static QList<AltitudeViolation> checkWaypoint(
        const Models::Waypoint& waypoint,
        const RegulatoryLimits& limits);

    /**
     * @brief Check if altitude exceeds regulatory maximum
     * @param altitude Altitude to check (meters AGL)
     * @param limits Regulatory limits
     * @return Violation if limit exceeded, null otherwise
     */
    static AltitudeViolation checkRegulatoryCompliance(
        double altitude,
        const Models::GeospatialCoordinate& location,
        const RegulatoryLimits& limits);

    /**
     * @brief Check terrain clearance
     * @param altitude Flight altitude (meters MSL)
     * @param terrainElevation Terrain elevation at location (meters MSL)
     * @param location Geospatial location
     * @param limits Regulatory limits
     * @return Violation if clearance insufficient
     */
    static AltitudeViolation checkTerrainClearance(
        double altitude,
        double terrainElevation,
        const Models::GeospatialCoordinate& location,
        const RegulatoryLimits& limits);

    /**
     * @brief Check if drone can safely fly at altitude
     * @param altitude Requested altitude (meters AGL)
     * @param droneModel Drone model
     * @param location Location
     * @return Violation if beyond drone capability
     */
    static AltitudeViolation checkDroneCapability(
        double altitude,
        Models::MissionParameters::CameraModel droneModel,
        const Models::GeospatialCoordinate& location);

    /**
     * @brief Calculate altitude above ground level
     * @param altitude Absolute altitude (MSL)
     * @param terrainElevation Terrain elevation (MSL)
     * @return Altitude AGL in meters
     */
    static double calculateAGL(double altitude, double terrainElevation);

    /**
     * @brief Get recommended safe altitude for location
     * @param location Geospatial location
     * @param terrainElevation Terrain elevation (MSL)
     * @param limits Regulatory limits
     * @return Recommended altitude (MSL)
     */
    static double getRecommendedAltitude(
        const Models::GeospatialCoordinate& location,
        double terrainElevation,
        const RegulatoryLimits& limits);

    /**
     * @brief Validate entire flight path for altitude consistency
     * @param waypoints List of waypoints
     * @return True if altitude changes are reasonable
     */
    static bool validateAltitudeProfile(
        const QList<Models::Waypoint>& waypoints);

    /**
     * @brief Get color code for altitude (for visualization)
     * @param altitude Altitude AGL
     * @param limits Regulatory limits
     * @return QColor for display
     */
    static QColor getAltitudeColor(
        double altitude,
        const RegulatoryLimits& limits);

private:
    static bool isViolationPresent(
        const AltitudeViolation& violation);

    static QString formatAltitude(double meters);

    static double getTerrainElevation(
        const Models::GeospatialCoordinate& location);
};

} // namespace Core
} // namespace DroneMapper

#endif // ALTITUDESAFETYCHECKER_H
