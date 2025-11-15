#ifndef BATTERYMANAGER_H
#define BATTERYMANAGER_H

#include "FlightPlan.h"
#include "MissionParameters.h"
#include <QList>
#include <QString>

namespace DroneMapper {
namespace Core {

/**
 * @brief Battery profile for different drone models
 */
struct BatteryProfile {
    QString droneName;
    double capacityMah;        // Battery capacity in mAh
    double nominalVoltage;     // Nominal voltage (V)
    double maxFlightTime;      // Maximum flight time (minutes)
    double safetyMargin;       // Safety margin (0.0-1.0, default 0.2 = 20%)
    double cruiseCurrentDraw;  // Average current draw in cruise (A)
    double hoverCurrentDraw;   // Current draw while hovering (A)

    double getUsableCapacity() const {
        return capacityMah * (1.0 - safetyMargin);
    }

    double getUsableFlightTime() const {
        return maxFlightTime * (1.0 - safetyMargin);
    }
};

/**
 * @brief Sub-mission created from battery split
 */
struct SubMission {
    int batteryNumber;              // Which battery (1, 2, 3...)
    QList<Models::Waypoint> waypoints;
    double estimatedFlightTime;     // Minutes
    double estimatedDistance;       // Meters
    int waypointStart;              // Start index in original plan
    int waypointEnd;                // End index in original plan
    Models::GeospatialCoordinate launchPoint;
    Models::GeospatialCoordinate landingPoint;
    QString notes;

    QString getSummary() const;
};

/**
 * @brief Manages battery usage and mission splitting
 *
 * Features:
 * - Automatic mission splitting for multi-battery operations
 * - Battery capacity modeling for different drones
 * - Flight time estimation based on mission profile
 * - Optimal return-to-home point selection
 * - Battery swap planning
 * - Safety margin enforcement
 * - Current draw estimation
 * - Energy consumption prediction
 */
class BatteryManager {
public:
    BatteryManager();

    /**
     * @brief Get battery profile for drone model
     */
    static BatteryProfile getBatteryProfile(
        Models::MissionParameters::CameraModel droneModel);

    /**
     * @brief Calculate estimated battery usage for flight plan
     * @param plan Flight plan to analyze
     * @param profile Battery profile to use
     * @return Estimated battery percentage used (0-100)
     */
    static double calculateBatteryUsage(
        const Models::FlightPlan& plan,
        const BatteryProfile& profile);

    /**
     * @brief Estimate flight time for mission
     * @param plan Flight plan
     * @param profile Battery profile
     * @return Estimated flight time in minutes
     */
    static double estimateFlightTime(
        const Models::FlightPlan& plan,
        const BatteryProfile& profile);

    /**
     * @brief Check if mission requires multiple batteries
     * @param plan Flight plan to check
     * @param profile Battery profile
     * @return Number of batteries required
     */
    static int calculateRequiredBatteries(
        const Models::FlightPlan& plan,
        const BatteryProfile& profile);

    /**
     * @brief Split mission into sub-missions for battery swaps
     * @param plan Original flight plan
     * @param profile Battery profile
     * @param homePoint Launch/landing location
     * @return List of sub-missions, one per battery
     */
    static QList<SubMission> splitMissionForBatteries(
        const Models::FlightPlan& plan,
        const BatteryProfile& profile,
        const Models::GeospatialCoordinate& homePoint);

    /**
     * @brief Find optimal RTH (return-to-home) point in waypoint sequence
     * @param waypoints List of waypoints
     * @param homePoint Home location
     * @param maxFlightTime Maximum flight time (minutes)
     * @param currentTime Time already used (minutes)
     * @return Waypoint index to return home from
     */
    static int findOptimalReturnPoint(
        const QList<Models::Waypoint>& waypoints,
        const Models::GeospatialCoordinate& homePoint,
        double maxFlightTime,
        double currentTime);

    /**
     * @brief Calculate energy consumption for mission segment
     * @param distance Distance to fly (meters)
     * @param altitude Flight altitude (meters)
     * @param speed Flight speed (m/s)
     * @param profile Battery profile
     * @return Energy consumption in Wh (watt-hours)
     */
    static double calculateEnergyConsumption(
        double distance,
        double altitude,
        double speed,
        const BatteryProfile& profile);

    /**
     * @brief Estimate current draw based on flight conditions
     * @param speed Flight speed (m/s)
     * @param altitude Altitude (meters)
     * @param windSpeed Wind speed (m/s)
     * @param profile Battery profile
     * @return Estimated current draw in amperes
     */
    static double estimateCurrentDraw(
        double speed,
        double altitude,
        double windSpeed,
        const BatteryProfile& profile);

    /**
     * @brief Get battery swap recommendation message
     * @param subMissions List of sub-missions
     * @return User-friendly battery swap instructions
     */
    static QString getBatterySwapInstructions(
        const QList<SubMission>& subMissions);

private:
    static double calculateSegmentTime(
        const Models::Waypoint& from,
        const Models::Waypoint& to,
        double speed);

    static double calculateReturnTime(
        const Models::GeospatialCoordinate& position,
        const Models::GeospatialCoordinate& home,
        double speed);
};

} // namespace Core
} // namespace DroneMapper

#endif // BATTERYMANAGER_H
