#include "AltitudeSafetyChecker.h"
#include "geospatial/GeoUtils.h"
#include <QtGui/QColor>
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace Core {

QString AltitudeViolation::getSeverityText() const
{
    switch (severity) {
    case ViolationSeverity::Safe:      return "SAFE";
    case ViolationSeverity::Warning:   return "WARNING";
    case ViolationSeverity::Caution:   return "CAUTION";
    case ViolationSeverity::Critical:  return "CRITICAL";
    case ViolationSeverity::Illegal:   return "ILLEGAL";
    }
    return "UNKNOWN";
}

QColor AltitudeViolation::getSeverityColor() const
{
    switch (severity) {
    case ViolationSeverity::Safe:      return QColor("#4CAF50"); // Green
    case ViolationSeverity::Warning:   return QColor("#FF9800"); // Orange
    case ViolationSeverity::Caution:   return QColor("#FFC107"); // Amber
    case ViolationSeverity::Critical:  return QColor("#F44336"); // Red
    case ViolationSeverity::Illegal:   return QColor("#9C27B0"); // Purple
    }
    return QColor("#9E9E9E"); // Gray
}

QString SafetyCheckResult::getDetailedReport() const
{
    QString report;
    report += QString("=== ALTITUDE SAFETY REPORT ===\n\n");
    report += QString("Overall Status: %1\n").arg(isSafe ? "SAFE" : "UNSAFE");
    report += QString("Max Altitude: %1 m AGL\n").arg(maxAltitude, 0, 'f', 1);
    report += QString("Min Ground Clearance: %1 m\n").arg(minGroundClearance, 0, 'f', 1);
    report += QString("Average Altitude: %1 m\n\n").arg(averageAltitude, 0, 'f', 1);

    if (violations.isEmpty()) {
        report += "No violations detected - safe to fly!\n";
    } else {
        report += QString("Violations Found: %1\n").arg(violations.count());
        report += QString("  Critical: %1\n").arg(criticalCount);
        report += QString("  Caution: %1\n").arg(cautionCount);
        report += QString("  Warning: %1\n\n").arg(warningCount);

        report += "VIOLATIONS:\n";
        for (const auto& v : violations) {
            report += QString("\n[%1] %2\n").arg(v.getSeverityText()).arg(v.type);
            report += QString("  %1\n").arg(v.description);
            if (v.waypointIndex >= 0) {
                report += QString("  Waypoint #%1\n").arg(v.waypointIndex + 1);
            }
            report += QString("  Altitude: %1 m (Limit: %2 m, Over by: %3 m)\n")
                .arg(v.altitude, 0, 'f', 1)
                .arg(v.limit, 0, 'f', 1)
                .arg(v.exceedance, 0, 'f', 1);
            report += QString("  Recommendation: %1\n").arg(v.recommendation);
        }
    }

    return report;
}

RegulatoryLimits RegulatoryLimits::getDefaults()
{
    return getFAA(); // Default to FAA Part 107 rules
}

RegulatoryLimits RegulatoryLimits::getFAA()
{
    RegulatoryLimits limits;
    limits.region = "USA (FAA Part 107)";
    limits.maxAltitudeAGL = 120.0;  // 400 feet = ~122m, using 120m for safety
    limits.maxAltitudeMSL = 0.0;    // No absolute limit, only AGL
    limits.minGroundClearance = 0.0; // No explicit minimum
    limits.minObstacleClearance = 0.0;
    limits.requiresVisualLineOfSight = true;
    limits.maxDistanceFromOperator = 0.0; // Varies, must maintain VLOS
    limits.notes = "FAA Part 107: Max 400 ft AGL, maintain VLOS, "
                   "can fly higher within 400 ft of structure";
    return limits;
}

RegulatoryLimits RegulatoryLimits::getEASA()
{
    RegulatoryLimits limits;
    limits.region = "EU (EASA)";
    limits.maxAltitudeAGL = 120.0;  // 120 meters AGL (Open category)
    limits.maxAltitudeMSL = 0.0;
    limits.minGroundClearance = 5.0; // 5 meters recommended
    limits.minObstacleClearance = 50.0; // 50 meters from people
    limits.requiresVisualLineOfSight = true;
    limits.maxDistanceFromOperator = 0.0;
    limits.notes = "EASA Open Category: Max 120m AGL, maintain VLOS, "
                   "keep 50m horizontal distance from uninvolved persons";
    return limits;
}

RegulatoryLimits RegulatoryLimits::getTransportCanada()
{
    RegulatoryLimits limits;
    limits.region = "Canada (Transport Canada)";
    limits.maxAltitudeAGL = 122.0;  // 400 feet AGL
    limits.maxAltitudeMSL = 0.0;
    limits.minGroundClearance = 0.0;
    limits.minObstacleClearance = 30.0; // 30 meters from others
    limits.requiresVisualLineOfSight = true;
    limits.maxDistanceFromOperator = 500.0; // 500 meters
    limits.notes = "Transport Canada: Max 400 ft AGL, maintain VLOS, "
                   "keep 30m from others, max 500m range";
    return limits;
}

DroneCapabilities DroneCapabilities::getCapabilities(
    Models::MissionParameters::CameraModel model)
{
    DroneCapabilities caps;

    switch (model) {
    case Models::MissionParameters::CameraModel::DJI_Mini3:
    case Models::MissionParameters::CameraModel::DJI_Mini3Pro:
        caps.maxAltitude = 3000.0;      // 3000m AGL max
        caps.maxFlightCeiling = 4000.0; // 4000m MSL
        caps.minSafeAltitude = 2.0;
        caps.recommendedAltitudeRange = 120.0;
        break;

    case Models::MissionParameters::CameraModel::DJI_Air3:
        caps.maxAltitude = 5000.0;      // 5000m AGL
        caps.maxFlightCeiling = 6000.0; // 6000m MSL
        caps.minSafeAltitude = 2.0;
        caps.recommendedAltitudeRange = 120.0;
        break;

    case Models::MissionParameters::CameraModel::DJI_Mavic3:
        caps.maxAltitude = 6000.0;      // 6000m AGL
        caps.maxFlightCeiling = 6000.0; // 6000m MSL
        caps.minSafeAltitude = 2.0;
        caps.recommendedAltitudeRange = 120.0;
        break;

    default:
        caps.maxAltitude = 500.0;
        caps.maxFlightCeiling = 1000.0;
        caps.minSafeAltitude = 2.0;
        caps.recommendedAltitudeRange = 100.0;
        break;
    }

    return caps;
}

AltitudeSafetyChecker::AltitudeSafetyChecker()
{
}

SafetyCheckResult AltitudeSafetyChecker::checkFlightPlan(
    const Models::FlightPlan& plan,
    const RegulatoryLimits& limits)
{
    SafetyCheckResult result;
    result.isSafe = true;
    result.criticalCount = 0;
    result.cautionCount = 0;
    result.warningCount = 0;
    result.maxAltitude = 0.0;
    result.minGroundClearance = 1e10;
    result.averageAltitude = 0.0;

    const auto& waypoints = plan.waypoints();

    if (waypoints.isEmpty()) {
        result.summary = "No waypoints to check";
        return result;
    }

    double totalAltitude = 0.0;

    // Check each waypoint
    for (int i = 0; i < waypoints.count(); ++i) {
        const auto& wp = waypoints[i];
        double altitude = wp.coordinate().altitude();
        totalAltitude += altitude;

        // Update statistics
        result.maxAltitude = std::max(result.maxAltitude, altitude);

        // Get terrain elevation (simplified - would use DEM in production)
        double terrainElevation = getTerrainElevation(wp.coordinate());
        double agl = calculateAGL(altitude, terrainElevation);
        double clearance = agl;
        result.minGroundClearance = std::min(result.minGroundClearance, clearance);

        // Check this waypoint
        auto wpViolations = checkWaypoint(wp, limits);
        for (auto& v : wpViolations) {
            v.waypointIndex = i;
            result.violations.append(v);

            // Count by severity
            switch (v.severity) {
            case ViolationSeverity::Critical:
            case ViolationSeverity::Illegal:
                result.criticalCount++;
                result.isSafe = false;
                break;
            case ViolationSeverity::Caution:
                result.cautionCount++;
                break;
            case ViolationSeverity::Warning:
                result.warningCount++;
                break;
            default:
                break;
            }
        }
    }

    result.averageAltitude = totalAltitude / waypoints.count();

    // Validate altitude profile
    if (!validateAltitudeProfile(waypoints)) {
        AltitudeViolation v;
        v.severity = ViolationSeverity::Warning;
        v.type = "Altitude Profile";
        v.description = "Erratic altitude changes detected - may cause poor image quality";
        v.altitude = result.maxAltitude;
        v.limit = result.averageAltitude;
        v.exceedance = 0.0;
        v.waypointIndex = -1;
        v.recommendation = "Review altitude consistency between waypoints";
        result.violations.append(v);
        result.warningCount++;
    }

    // Generate summary
    if (result.isSafe) {
        result.summary = QString("Flight plan is SAFE - %1 waypoints checked, max altitude %2m AGL")
            .arg(waypoints.count())
            .arg(result.maxAltitude, 0, 'f', 1);
    } else {
        result.summary = QString("UNSAFE - %1 critical violations found, DO NOT FLY")
            .arg(result.criticalCount);
    }

    return result;
}

QList<AltitudeViolation> AltitudeSafetyChecker::checkWaypoint(
    const Models::Waypoint& waypoint,
    const RegulatoryLimits& limits)
{
    QList<AltitudeViolation> violations;

    double altitude = waypoint.coordinate().altitude();
    const auto& location = waypoint.coordinate();

    // Check regulatory compliance
    AltitudeViolation regViolation = checkRegulatoryCompliance(
        altitude, location, limits);
    if (isViolationPresent(regViolation)) {
        violations.append(regViolation);
    }

    // Check terrain clearance
    double terrainElevation = getTerrainElevation(location);
    AltitudeViolation terrainViolation = checkTerrainClearance(
        altitude, terrainElevation, location, limits);
    if (isViolationPresent(terrainViolation)) {
        violations.append(terrainViolation);
    }

    return violations;
}

AltitudeViolation AltitudeSafetyChecker::checkRegulatoryCompliance(
    double altitude,
    const Models::GeospatialCoordinate& location,
    const RegulatoryLimits& limits)
{
    AltitudeViolation violation;
    violation.severity = ViolationSeverity::Safe;
    violation.type = "Regulatory Compliance";
    violation.location = location;
    violation.altitude = altitude;
    violation.limit = limits.maxAltitudeAGL;
    violation.waypointIndex = -1;

    // Check if altitude exceeds regulatory maximum
    if (altitude > limits.maxAltitudeAGL) {
        violation.severity = ViolationSeverity::Illegal;
        violation.exceedance = altitude - limits.maxAltitudeAGL;
        violation.description = QString("Altitude %1m exceeds %2 maximum of %3m AGL")
            .arg(altitude, 0, 'f', 1)
            .arg(limits.region)
            .arg(limits.maxAltitudeAGL, 0, 'f', 1);
        violation.recommendation = QString("Reduce altitude to %1m or below")
            .arg(limits.maxAltitudeAGL, 0, 'f', 1);
    } else if (altitude > limits.maxAltitudeAGL * 0.9) {
        // Warning if within 10% of limit
        violation.severity = ViolationSeverity::Warning;
        violation.exceedance = 0.0;
        violation.description = QString("Altitude %1m is close to regulatory limit of %2m")
            .arg(altitude, 0, 'f', 1)
            .arg(limits.maxAltitudeAGL, 0, 'f', 1);
        violation.recommendation = "Consider adding safety margin to altitude";
    }

    return violation;
}

AltitudeViolation AltitudeSafetyChecker::checkTerrainClearance(
    double altitude,
    double terrainElevation,
    const Models::GeospatialCoordinate& location,
    const RegulatoryLimits& limits)
{
    AltitudeViolation violation;
    violation.severity = ViolationSeverity::Safe;
    violation.type = "Terrain Clearance";
    violation.location = location;
    violation.altitude = altitude;
    violation.waypointIndex = -1;

    double agl = calculateAGL(altitude, terrainElevation);
    double minClearance = std::max(limits.minGroundClearance, 5.0); // Minimum 5m recommended

    violation.limit = minClearance;

    if (agl < minClearance) {
        violation.severity = ViolationSeverity::Critical;
        violation.exceedance = minClearance - agl;
        violation.description = QString("Insufficient terrain clearance: %1m (minimum %2m)")
            .arg(agl, 0, 'f', 1)
            .arg(minClearance, 0, 'f', 1);
        violation.recommendation = QString("Increase altitude by at least %1m")
            .arg(violation.exceedance, 0, 'f', 1);
    } else if (agl < minClearance * 1.5) {
        violation.severity = ViolationSeverity::Caution;
        violation.exceedance = 0.0;
        violation.description = QString("Low terrain clearance: %1m")
            .arg(agl, 0, 'f', 1);
        violation.recommendation = "Increase altitude for safer clearance";
    }

    return violation;
}

AltitudeViolation AltitudeSafetyChecker::checkDroneCapability(
    double altitude,
    Models::MissionParameters::CameraModel droneModel,
    const Models::GeospatialCoordinate& location)
{
    AltitudeViolation violation;
    violation.severity = ViolationSeverity::Safe;
    violation.type = "Drone Capability";
    violation.location = location;
    violation.altitude = altitude;
    violation.waypointIndex = -1;

    DroneCapabilities caps = DroneCapabilities::getCapabilities(droneModel);
    violation.limit = caps.maxAltitude;

    if (altitude > caps.maxAltitude) {
        violation.severity = ViolationSeverity::Critical;
        violation.exceedance = altitude - caps.maxAltitude;
        violation.description = QString("Altitude %1m exceeds drone maximum of %2m")
            .arg(altitude, 0, 'f', 1)
            .arg(caps.maxAltitude, 0, 'f', 1);
        violation.recommendation = QString("Reduce altitude to %1m or below")
            .arg(caps.maxAltitude, 0, 'f', 1);
    }

    return violation;
}

double AltitudeSafetyChecker::calculateAGL(double altitude, double terrainElevation)
{
    return altitude - terrainElevation;
}

double AltitudeSafetyChecker::getRecommendedAltitude(
    const Models::GeospatialCoordinate& location,
    double terrainElevation,
    const RegulatoryLimits& limits)
{
    // Recommended altitude is 75% of maximum, ensuring safe margin
    double recommendedAGL = limits.maxAltitudeAGL * 0.75;
    return terrainElevation + recommendedAGL;
}

bool AltitudeSafetyChecker::validateAltitudeProfile(
    const QList<Models::Waypoint>& waypoints)
{
    if (waypoints.count() < 3) {
        return true; // Too few points to validate
    }

    // Check for excessive altitude changes between waypoints
    double maxChange = 0.0;
    for (int i = 1; i < waypoints.count(); ++i) {
        double change = std::abs(waypoints[i].coordinate().altitude() - waypoints[i-1].coordinate().altitude());
        maxChange = std::max(maxChange, change);
    }

    // Flag if any single change exceeds 50m
    if (maxChange > 50.0) {
        return false;
    }

    // Check altitude variance
    double sum = 0.0;
    for (const auto& wp : waypoints) {
        sum += wp.coordinate().altitude();
    }
    double mean = sum / waypoints.count();

    double variance = 0.0;
    for (const auto& wp : waypoints) {
        double diff = wp.coordinate().altitude() - mean;
        variance += diff * diff;
    }
    variance /= waypoints.count();
    double stddev = std::sqrt(variance);

    // Flag if standard deviation is too high (indicates erratic altitude)
    return stddev < 30.0;
}

QColor AltitudeSafetyChecker::getAltitudeColor(
    double altitude,
    const RegulatoryLimits& limits)
{
    double ratio = altitude / limits.maxAltitudeAGL;

    if (ratio > 1.0) {
        return QColor("#F44336"); // Red - illegal
    } else if (ratio > 0.9) {
        return QColor("#FF9800"); // Orange - warning
    } else if (ratio > 0.75) {
        return QColor("#FFC107"); // Amber - caution
    } else if (ratio > 0.5) {
        return QColor("#8BC34A"); // Light green - good
    } else {
        return QColor("#4CAF50"); // Green - safe
    }
}

bool AltitudeSafetyChecker::isViolationPresent(
    const AltitudeViolation& violation)
{
    return violation.severity != ViolationSeverity::Safe;
}

QString AltitudeSafetyChecker::formatAltitude(double meters)
{
    return QString("%1 m (%2 ft)")
        .arg(meters, 0, 'f', 1)
        .arg(meters * 3.28084, 0, 'f', 0); // Convert to feet
}

double AltitudeSafetyChecker::getTerrainElevation(
    const Models::GeospatialCoordinate& location)
{
    // Simplified - in production this would query a DEM (Digital Elevation Model)
    // For now, assume flat terrain at 0m MSL
    // TODO: Integrate with SRTM or other DEM data
    Q_UNUSED(location);
    return 0.0;
}

} // namespace Core
} // namespace DroneMapper
