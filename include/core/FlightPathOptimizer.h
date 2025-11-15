#ifndef FLIGHTPATHOPTIMIZER_H
#define FLIGHTPATHOPTIMIZER_H

#include "FlightPlan.h"
#include "MissionParameters.h"
#include "GeospatialCoordinate.h"
#include <QList>
#include <QString>
#include <QPair>

namespace DroneMapper {
namespace Core {

/**
 * @brief Optimization metrics and statistics
 */
struct OptimizationResult {
    QList<Models::Waypoint> optimizedWaypoints;
    double originalDistance;      // Meters
    double optimizedDistance;     // Meters
    double distanceSaved;         // Meters
    double timeSaved;             // Minutes
    double percentageImprovement; // 0-100%
    int waypointChanges;          // Number of waypoints reordered
    int directionChanges;         // Number of direction changes
    QString optimizationMethod;   // Description of method used
    QString summary;              // Human-readable summary
};

/**
 * @brief Path optimization configuration
 */
struct OptimizationConfig {
    bool preserveFirstWaypoint;   // Keep first waypoint as start
    bool preserveLastWaypoint;    // Keep last waypoint as end
    bool minimizeTurns;           // Reduce direction changes
    bool optimizeForWind;         // Consider wind direction
    double windDirection;         // Wind direction in degrees (0-360)
    double windSpeed;             // Wind speed in m/s
    int maxIterations;            // Maximum optimization iterations
    QString strategy;             // "greedy", "2-opt", "grid-aware"

    OptimizationConfig()
        : preserveFirstWaypoint(true)
        , preserveLastWaypoint(false)
        , minimizeTurns(true)
        , optimizeForWind(false)
        , windDirection(0.0)
        , windSpeed(0.0)
        , maxIterations(1000)
        , strategy("greedy")
    {}
};

/**
 * @brief Optimizes flight paths for efficiency and quality
 *
 * Features:
 * - Waypoint order optimization (TSP-based)
 * - Direction change minimization
 * - Wind-aware path planning
 * - Grid pattern detection and preservation
 * - 2-opt local search refinement
 * - Distance and time savings calculation
 * - Photogrammetry coverage validation
 *
 * Algorithms:
 * - Nearest Neighbor (greedy)
 * - 2-opt local search
 * - Grid-aware optimization (preserves survey patterns)
 * - Wind-compensated routing
 */
class FlightPathOptimizer {
public:
    FlightPathOptimizer();

    /**
     * @brief Optimize waypoint order in flight plan
     * @param plan Original flight plan
     * @param config Optimization configuration
     * @return Optimization result with new waypoint order and metrics
     */
    static OptimizationResult optimizeFlightPlan(
        const Models::FlightPlan& plan,
        const OptimizationConfig& config = OptimizationConfig());

    /**
     * @brief Optimize waypoint list
     * @param waypoints List of waypoints to optimize
     * @param startPoint Starting location (launch point)
     * @param config Optimization configuration
     * @return Optimization result
     */
    static OptimizationResult optimizeWaypoints(
        const QList<Models::Waypoint>& waypoints,
        const Models::GeospatialCoordinate& startPoint,
        const OptimizationConfig& config = OptimizationConfig());

    /**
     * @brief Calculate total path distance
     * @param waypoints Ordered list of waypoints
     * @return Total distance in meters
     */
    static double calculateTotalDistance(
        const QList<Models::Waypoint>& waypoints);

    /**
     * @brief Calculate number of direction changes
     * @param waypoints Ordered list of waypoints
     * @param turnThreshold Minimum angle change to count as turn (degrees)
     * @return Number of significant direction changes
     */
    static int calculateDirectionChanges(
        const QList<Models::Waypoint>& waypoints,
        double turnThreshold = 30.0);

    /**
     * @brief Detect if waypoints form a grid pattern
     * @param waypoints List of waypoints
     * @return True if waypoints appear to be in a survey grid
     */
    static bool isGridPattern(const QList<Models::Waypoint>& waypoints);

    /**
     * @brief Calculate optimal flight direction based on wind
     * @param windDirection Wind direction (degrees, 0-360)
     * @param windSpeed Wind speed (m/s)
     * @return Recommended flight heading (degrees)
     */
    static double calculateOptimalHeading(
        double windDirection,
        double windSpeed);

    /**
     * @brief Estimate time savings from optimization
     * @param distanceSaved Distance reduction (meters)
     * @param speed Flight speed (m/s)
     * @param turnsReduced Number of turns eliminated
     * @return Time savings in minutes
     */
    static double estimateTimeSavings(
        double distanceSaved,
        double speed,
        int turnsReduced);

    /**
     * @brief Validate that optimized path maintains coverage
     * @param original Original waypoints
     * @param optimized Optimized waypoints
     * @return True if coverage is equivalent
     */
    static bool validateCoverage(
        const QList<Models::Waypoint>& original,
        const QList<Models::Waypoint>& optimized);

private:
    // Optimization algorithms
    static QList<Models::Waypoint> greedyNearestNeighbor(
        const QList<Models::Waypoint>& waypoints,
        const Models::GeospatialCoordinate& start,
        const OptimizationConfig& config);

    static QList<Models::Waypoint> twoOptOptimization(
        const QList<Models::Waypoint>& waypoints,
        const OptimizationConfig& config);

    static QList<Models::Waypoint> gridAwareOptimization(
        const QList<Models::Waypoint>& waypoints,
        const OptimizationConfig& config);

    static QList<Models::Waypoint> windAwareOptimization(
        const QList<Models::Waypoint>& waypoints,
        const OptimizationConfig& config);

    // Helper functions
    static double calculateBearing(
        const Models::GeospatialCoordinate& from,
        const Models::GeospatialCoordinate& to);

    static double calculateTurnAngle(
        double bearing1,
        double bearing2);

    static double windPenalty(
        const Models::GeospatialCoordinate& from,
        const Models::GeospatialCoordinate& to,
        double windDirection,
        double windSpeed);

    static int findNearestWaypoint(
        const Models::GeospatialCoordinate& position,
        const QList<Models::Waypoint>& waypoints,
        const QList<bool>& visited);

    static bool shouldSwap(
        const QList<Models::Waypoint>& waypoints,
        int i,
        int j,
        const OptimizationConfig& config);

    static double calculateSegmentCost(
        const Models::Waypoint& from,
        const Models::Waypoint& to,
        const OptimizationConfig& config);
};

} // namespace Core
} // namespace DroneMapper

#endif // FLIGHTPATHOPTIMIZER_H
