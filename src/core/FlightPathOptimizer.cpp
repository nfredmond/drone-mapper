#include "FlightPathOptimizer.h"
#include "geospatial/GeoUtils.h"
#include <cmath>
#include <algorithm>
#include <QSet>

namespace DroneMapper {
namespace Core {

FlightPathOptimizer::FlightPathOptimizer()
{
}

OptimizationResult FlightPathOptimizer::optimizeFlightPlan(
    const Models::FlightPlan& plan,
    const OptimizationConfig& config)
{
    const auto& waypoints = plan.waypoints();
    if (waypoints.isEmpty()) {
        OptimizationResult result;
        result.optimizedWaypoints = waypoints;
        result.originalDistance = 0.0;
        result.optimizedDistance = 0.0;
        result.distanceSaved = 0.0;
        result.timeSaved = 0.0;
        result.percentageImprovement = 0.0;
        result.waypointChanges = 0;
        result.directionChanges = 0;
        result.optimizationMethod = "None (empty plan)";
        result.summary = "No waypoints to optimize";
        return result;
    }

    Models::GeospatialCoordinate startPoint = waypoints.first().coordinate();
    return optimizeWaypoints(waypoints, startPoint, config);
}

OptimizationResult FlightPathOptimizer::optimizeWaypoints(
    const QList<Models::Waypoint>& waypoints,
    const Models::GeospatialCoordinate& startPoint,
    const OptimizationConfig& config)
{
    OptimizationResult result;

    if (waypoints.count() < 3) {
        // Too few waypoints to optimize
        result.optimizedWaypoints = waypoints;
        result.originalDistance = calculateTotalDistance(waypoints);
        result.optimizedDistance = result.originalDistance;
        result.distanceSaved = 0.0;
        result.timeSaved = 0.0;
        result.percentageImprovement = 0.0;
        result.waypointChanges = 0;
        result.directionChanges = calculateDirectionChanges(waypoints);
        result.optimizationMethod = "None (too few waypoints)";
        result.summary = "Path too short to optimize";
        return result;
    }

    // Calculate original metrics
    result.originalDistance = calculateTotalDistance(waypoints);
    int originalTurns = calculateDirectionChanges(waypoints);

    // Choose optimization strategy
    QList<Models::Waypoint> optimized;

    if (config.strategy == "grid-aware" || isGridPattern(waypoints)) {
        optimized = gridAwareOptimization(waypoints, config);
        result.optimizationMethod = "Grid-aware optimization";
    } else if (config.optimizeForWind && config.windSpeed > 3.0) {
        optimized = windAwareOptimization(waypoints, config);
        result.optimizationMethod = "Wind-aware optimization";
    } else {
        // Standard greedy nearest neighbor + 2-opt
        optimized = greedyNearestNeighbor(waypoints, startPoint, config);
        optimized = twoOptOptimization(optimized, config);
        result.optimizationMethod = "Nearest Neighbor + 2-opt";
    }

    // Calculate optimized metrics
    result.optimizedWaypoints = optimized;
    result.optimizedDistance = calculateTotalDistance(optimized);
    result.distanceSaved = result.originalDistance - result.optimizedDistance;
    result.percentageImprovement = (result.distanceSaved / result.originalDistance) * 100.0;
    result.directionChanges = calculateDirectionChanges(optimized);

    // Count waypoint reordering
    result.waypointChanges = 0;
    for (int i = 0; i < waypoints.count() && i < optimized.count(); ++i) {
        const auto& orig = waypoints[i].coordinate();
        const auto& opt = optimized[i].coordinate();
        if (orig.latitude() != opt.latitude() || orig.longitude() != opt.longitude()) {
            result.waypointChanges++;
        }
    }

    // Estimate time savings (assuming 10 m/s flight speed)
    int turnsReduced = originalTurns - result.directionChanges;
    result.timeSaved = estimateTimeSavings(result.distanceSaved, 10.0, turnsReduced);

    // Generate summary
    result.summary = QString("Optimized path saves %1 m (%2%) and %3 minutes")
        .arg(result.distanceSaved, 0, 'f', 0)
        .arg(result.percentageImprovement, 0, 'f', 1)
        .arg(result.timeSaved, 0, 'f', 1);

    return result;
}

double FlightPathOptimizer::calculateTotalDistance(
    const QList<Models::Waypoint>& waypoints)
{
    if (waypoints.count() < 2) {
        return 0.0;
    }

    double total = 0.0;
    for (int i = 1; i < waypoints.count(); ++i) {
        total += Geospatial::GeoUtils::distanceBetween(
            waypoints[i-1].coordinate(),
            waypoints[i].coordinate());
    }

    return total;
}

int FlightPathOptimizer::calculateDirectionChanges(
    const QList<Models::Waypoint>& waypoints,
    double turnThreshold)
{
    if (waypoints.count() < 3) {
        return 0;
    }

    int changes = 0;
    double prevBearing = calculateBearing(
        waypoints[0].coordinate(),
        waypoints[1].coordinate());

    for (int i = 2; i < waypoints.count(); ++i) {
        double bearing = calculateBearing(
            waypoints[i-1].coordinate(),
            waypoints[i].coordinate());

        double turnAngle = calculateTurnAngle(prevBearing, bearing);

        if (turnAngle > turnThreshold) {
            changes++;
        }

        prevBearing = bearing;
    }

    return changes;
}

bool FlightPathOptimizer::isGridPattern(const QList<Models::Waypoint>& waypoints)
{
    if (waypoints.count() < 4) {
        return false;
    }

    // Check if waypoints have regular spacing and alternating directions
    // This is a simplified grid detection
    QList<double> bearings;
    for (int i = 1; i < qMin(waypoints.count(), 10); ++i) {
        double bearing = calculateBearing(
            waypoints[i-1].coordinate(),
            waypoints[i].coordinate());
        bearings.append(bearing);
    }

    // Check for alternating directions (parallel survey lines)
    int alternations = 0;
    for (int i = 1; i < bearings.count(); ++i) {
        double diff = std::abs(calculateTurnAngle(bearings[i-1], bearings[i]));
        if (diff > 150.0 && diff < 210.0) { // ~180 degree turn
            alternations++;
        }
    }

    // If more than 30% of turns are 180-degree reversals, it's likely a grid
    return (alternations > bearings.count() * 0.3);
}

double FlightPathOptimizer::calculateOptimalHeading(
    double windDirection,
    double windSpeed)
{
    // Optimal heading is typically into the wind or perpendicular
    // For photogrammetry, prefer headwind/tailwind to crosswind
    // This returns the direction for best stability
    return windDirection; // Fly into wind
}

double FlightPathOptimizer::estimateTimeSavings(
    double distanceSaved,
    double speed,
    int turnsReduced)
{
    if (speed <= 0.0) speed = 10.0;

    // Time saved from distance reduction
    double timeSavedDistance = distanceSaved / speed; // seconds

    // Time saved from turn reduction (assume 5 seconds per turn)
    double timeSavedTurns = turnsReduced * 5.0; // seconds

    // Convert to minutes
    return (timeSavedDistance + timeSavedTurns) / 60.0;
}

bool FlightPathOptimizer::validateCoverage(
    const QList<Models::Waypoint>& original,
    const QList<Models::Waypoint>& optimized)
{
    // Check that all waypoints are preserved (order may differ)
    if (original.count() != optimized.count()) {
        return false;
    }

    // Create sets of coordinates
    QSet<QString> originalSet;
    QSet<QString> optimizedSet;

    for (const auto& wp : original) {
        QString key = QString("%1,%2")
            .arg(wp.coordinate().latitude(), 0, 'f', 8)
            .arg(wp.coordinate().longitude(), 0, 'f', 8);
        originalSet.insert(key);
    }

    for (const auto& wp : optimized) {
        QString key = QString("%1,%2")
            .arg(wp.coordinate().latitude(), 0, 'f', 8)
            .arg(wp.coordinate().longitude(), 0, 'f', 8);
        optimizedSet.insert(key);
    }

    return originalSet == optimizedSet;
}

QList<Models::Waypoint> FlightPathOptimizer::greedyNearestNeighbor(
    const QList<Models::Waypoint>& waypoints,
    const Models::GeospatialCoordinate& start,
    const OptimizationConfig& config)
{
    QList<Models::Waypoint> result;
    QList<bool> visited(waypoints.count(), false);

    // Start from nearest waypoint to start point
    int currentIdx = findNearestWaypoint(start, waypoints, visited);
    if (currentIdx == -1) return waypoints;

    if (config.preserveFirstWaypoint) {
        // Keep first waypoint as-is
        currentIdx = 0;
    }

    visited[currentIdx] = true;
    result.append(waypoints[currentIdx]);

    // Greedy selection
    for (int i = 1; i < waypoints.count(); ++i) {
        int nextIdx = findNearestWaypoint(
            result.last().coordinate(),
            waypoints,
            visited);

        if (nextIdx == -1) break;

        visited[nextIdx] = true;
        result.append(waypoints[nextIdx]);
    }

    return result;
}

QList<Models::Waypoint> FlightPathOptimizer::twoOptOptimization(
    const QList<Models::Waypoint>& waypoints,
    const OptimizationConfig& config)
{
    if (waypoints.count() < 4) {
        return waypoints;
    }

    QList<Models::Waypoint> result = waypoints;
    bool improved = true;
    int iterations = 0;

    while (improved && iterations < config.maxIterations) {
        improved = false;
        iterations++;

        for (int i = 1; i < result.count() - 2; ++i) {
            for (int j = i + 1; j < result.count() - 1; ++j) {
                if (shouldSwap(result, i, j, config)) {
                    // Reverse segment between i and j
                    int left = i;
                    int right = j;
                    while (left < right) {
                        std::swap(result[left], result[right]);
                        left++;
                        right--;
                    }
                    improved = true;
                }
            }
        }
    }

    return result;
}

QList<Models::Waypoint> FlightPathOptimizer::gridAwareOptimization(
    const QList<Models::Waypoint>& waypoints,
    const OptimizationConfig& config)
{
    // For grid patterns, preserve the structure but optimize line order
    // This maintains photogrammetry coverage while reducing travel time

    // For now, return original order with minor 2-opt refinement
    // A full implementation would detect grid lines and optimize their sequence
    return twoOptOptimization(waypoints, config);
}

QList<Models::Waypoint> FlightPathOptimizer::windAwareOptimization(
    const QList<Models::Waypoint>& waypoints,
    const OptimizationConfig& config)
{
    // Prefer flying into wind on longer segments
    // Use modified nearest neighbor with wind penalty

    QList<Models::Waypoint> result;
    QList<bool> visited(waypoints.count(), false);

    // Start from first waypoint
    int currentIdx = 0;
    if (config.preserveFirstWaypoint) {
        visited[0] = true;
        result.append(waypoints[0]);
        currentIdx = 0;
    }

    // Select next waypoint considering wind
    while (result.count() < waypoints.count()) {
        double bestCost = 1e10;
        int bestIdx = -1;

        for (int i = 0; i < waypoints.count(); ++i) {
            if (visited[i]) continue;

            double cost = calculateSegmentCost(
                result.isEmpty() ? waypoints[0] : result.last(),
                waypoints[i],
                config);

            if (cost < bestCost) {
                bestCost = cost;
                bestIdx = i;
            }
        }

        if (bestIdx == -1) break;

        visited[bestIdx] = true;
        result.append(waypoints[bestIdx]);
    }

    return result;
}

double FlightPathOptimizer::calculateBearing(
    const Models::GeospatialCoordinate& from,
    const Models::GeospatialCoordinate& to)
{
    double lat1 = from.latitude() * M_PI / 180.0;
    double lat2 = to.latitude() * M_PI / 180.0;
    double dLon = (to.longitude() - from.longitude()) * M_PI / 180.0;

    double y = std::sin(dLon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) -
               std::sin(lat1) * std::cos(lat2) * std::cos(dLon);

    double bearing = std::atan2(y, x) * 180.0 / M_PI;
    return std::fmod(bearing + 360.0, 360.0);
}

double FlightPathOptimizer::calculateTurnAngle(
    double bearing1,
    double bearing2)
{
    double diff = std::abs(bearing2 - bearing1);
    if (diff > 180.0) {
        diff = 360.0 - diff;
    }
    return diff;
}

double FlightPathOptimizer::windPenalty(
    const Models::GeospatialCoordinate& from,
    const Models::GeospatialCoordinate& to,
    double windDirection,
    double windSpeed)
{
    // Calculate bearing of flight path
    double pathBearing = calculateBearing(from, to);

    // Calculate angle difference between path and wind
    double windAngle = calculateTurnAngle(pathBearing, windDirection + 180.0);

    // Penalty is higher for headwinds (flying into wind)
    // Lower for tailwinds (wind behind)
    double penalty = std::cos(windAngle * M_PI / 180.0) * windSpeed;

    return penalty;
}

int FlightPathOptimizer::findNearestWaypoint(
    const Models::GeospatialCoordinate& position,
    const QList<Models::Waypoint>& waypoints,
    const QList<bool>& visited)
{
    double minDistance = 1e10;
    int nearestIdx = -1;

    for (int i = 0; i < waypoints.count(); ++i) {
        if (visited[i]) continue;

        double distance = Geospatial::GeoUtils::distanceBetween(
            position,
            waypoints[i].coordinate());

        if (distance < minDistance) {
            minDistance = distance;
            nearestIdx = i;
        }
    }

    return nearestIdx;
}

bool FlightPathOptimizer::shouldSwap(
    const QList<Models::Waypoint>& waypoints,
    int i,
    int j,
    const OptimizationConfig& config)
{
    // Calculate current path distance
    double currentDist =
        Geospatial::GeoUtils::distanceBetween(
            waypoints[i-1].coordinate(),
            waypoints[i].coordinate()) +
        Geospatial::GeoUtils::distanceBetween(
            waypoints[j].coordinate(),
            waypoints[j+1].coordinate());

    // Calculate swapped path distance
    double swappedDist =
        Geospatial::GeoUtils::distanceBetween(
            waypoints[i-1].coordinate(),
            waypoints[j].coordinate()) +
        Geospatial::GeoUtils::distanceBetween(
            waypoints[i].coordinate(),
            waypoints[j+1].coordinate());

    return swappedDist < currentDist;
}

double FlightPathOptimizer::calculateSegmentCost(
    const Models::Waypoint& from,
    const Models::Waypoint& to,
    const OptimizationConfig& config)
{
    double distance = Geospatial::GeoUtils::distanceBetween(
        from.coordinate(),
        to.coordinate());

    double cost = distance;

    // Add wind penalty if wind optimization enabled
    if (config.optimizeForWind) {
        double penalty = windPenalty(
            from.coordinate(),
            to.coordinate(),
            config.windDirection,
            config.windSpeed);
        cost += penalty * 10.0; // Scale factor
    }

    return cost;
}

} // namespace Core
} // namespace DroneMapper
