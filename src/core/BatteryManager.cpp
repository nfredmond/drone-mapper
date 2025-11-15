#include "BatteryManager.h"
#include "geospatial/GeoUtils.h"
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace Core {

QString SubMission::getSummary() const
{
    return QString("Battery %1: %2 waypoints, %3 min flight, %4 m distance")
        .arg(batteryNumber)
        .arg(waypoints.count())
        .arg(estimatedFlightTime, 0, 'f', 1)
        .arg(estimatedDistance, 0, 'f', 0);
}

BatteryManager::BatteryManager()
{
}

BatteryProfile BatteryManager::getBatteryProfile(
    Models::MissionParameters::CameraModel droneModel)
{
    BatteryProfile profile;
    profile.safetyMargin = 0.20; // 20% safety margin (industry standard)

    switch (droneModel) {
    case Models::MissionParameters::CameraModel::DJI_Mini3:
        profile.droneName = "DJI Mini 3";
        profile.capacityMah = 2453;
        profile.nominalVoltage = 7.38;
        profile.maxFlightTime = 38.0; // Optimal conditions
        profile.cruiseCurrentDraw = 3.5;
        profile.hoverCurrentDraw = 2.8;
        break;

    case Models::MissionParameters::CameraModel::DJI_Mini3Pro:
        profile.droneName = "DJI Mini 3 Pro";
        profile.capacityMah = 2453;
        profile.nominalVoltage = 7.38;
        profile.maxFlightTime = 34.0; // With camera
        profile.cruiseCurrentDraw = 3.8;
        profile.hoverCurrentDraw = 3.0;
        break;

    case Models::MissionParameters::CameraModel::DJI_Air3:
        profile.droneName = "DJI Air 3";
        profile.capacityMah = 4241;
        profile.nominalVoltage = 11.55;
        profile.maxFlightTime = 46.0; // No wind
        profile.cruiseCurrentDraw = 5.2;
        profile.hoverCurrentDraw = 4.0;
        break;

    case Models::MissionParameters::CameraModel::DJI_Mavic3:
        profile.droneName = "DJI Mavic 3";
        profile.capacityMah = 5000;
        profile.nominalVoltage = 15.4;
        profile.maxFlightTime = 46.0; // Professional use
        profile.cruiseCurrentDraw = 6.0;
        profile.hoverCurrentDraw = 4.5;
        break;

    default:
        // Conservative default profile
        profile.droneName = "Generic Drone";
        profile.capacityMah = 3000;
        profile.nominalVoltage = 11.1;
        profile.maxFlightTime = 25.0;
        profile.cruiseCurrentDraw = 4.0;
        profile.hoverCurrentDraw = 3.2;
        break;
    }

    return profile;
}

double BatteryManager::calculateBatteryUsage(
    const Models::FlightPlan& plan,
    const BatteryProfile& profile)
{
    double flightTime = estimateFlightTime(plan, profile);
    double maxTime = profile.getUsableFlightTime();

    if (maxTime <= 0.0) return 100.0;

    return (flightTime / maxTime) * 100.0;
}

double BatteryManager::estimateFlightTime(
    const Models::FlightPlan& plan,
    const BatteryProfile& profile)
{
    if (plan.waypoints().isEmpty()) {
        return 0.0;
    }

    const auto& waypoints = plan.waypoints();
    const auto& params = plan.parameters();
    double speed = params.flightSpeed();

    // Calculate flight time
    double totalTime = 0.0;

    // Time between waypoints
    for (int i = 1; i < waypoints.count(); ++i) {
        totalTime += calculateSegmentTime(waypoints[i-1], waypoints[i], speed);
    }

    // Add takeoff time (30 seconds)
    totalTime += 0.5;

    // Add landing time (30 seconds)
    totalTime += 0.5;

    // Add photo capture overhead (15% of flight time)
    totalTime *= 1.15;

    // Add turn overhead (10% for direction changes)
    totalTime *= 1.10;

    return totalTime;
}

int BatteryManager::calculateRequiredBatteries(
    const Models::FlightPlan& plan,
    const BatteryProfile& profile)
{
    double flightTime = estimateFlightTime(plan, profile);
    double usableTime = profile.getUsableFlightTime();

    if (usableTime <= 0.0) return 1;

    // Account for return-to-home time between battery swaps
    // Estimate 2-3 minutes RTH overhead per battery swap
    double rthOverhead = 2.5; // minutes

    int batteries = 1;
    double remainingTime = flightTime;
    double availableTime = usableTime;

    while (remainingTime > availableTime) {
        batteries++;
        remainingTime -= availableTime;
        availableTime = usableTime - rthOverhead; // Subsequent batteries need RTH time
    }

    return batteries;
}

QList<SubMission> BatteryManager::splitMissionForBatteries(
    const Models::FlightPlan& plan,
    const BatteryProfile& profile,
    const Models::GeospatialCoordinate& homePoint)
{
    QList<SubMission> subMissions;

    const auto& waypoints = plan.waypoints();
    const auto& params = plan.parameters();

    if (waypoints.isEmpty()) {
        return subMissions;
    }

    double speed = params.flightSpeed();
    double usableTime = profile.getUsableFlightTime();

    int numBatteries = calculateRequiredBatteries(plan, profile);

    if (numBatteries == 1) {
        // Single battery mission - return whole plan
        SubMission mission;
        mission.batteryNumber = 1;
        mission.waypoints = waypoints;
        mission.estimatedFlightTime = estimateFlightTime(plan, profile);
        mission.estimatedDistance = plan.totalDistance();
        mission.waypointStart = 0;
        mission.waypointEnd = waypoints.count() - 1;
        mission.launchPoint = homePoint;
        mission.landingPoint = homePoint;
        mission.notes = "Complete mission on single battery";
        subMissions.append(mission);
        return subMissions;
    }

    // Multi-battery mission - split intelligently
    int currentWaypoint = 0;
    double currentTime = 0.5; // Takeoff time

    for (int batteryNum = 1; batteryNum <= numBatteries; ++batteryNum) {
        SubMission mission;
        mission.batteryNumber = batteryNum;
        mission.waypointStart = currentWaypoint;
        mission.launchPoint = (batteryNum == 1) ? homePoint :
            waypoints[currentWaypoint].coordinate();

        double availableTime = usableTime;
        if (batteryNum > 1) {
            // Account for RTH overhead on subsequent batteries
            availableTime -= 2.5; // 2.5 min RTH buffer
        }

        // Find how many waypoints we can do on this battery
        double segmentTime = 0.0;
        int waypointsThisBattery = 0;
        double distanceThisBattery = 0.0;

        while (currentWaypoint < waypoints.count()) {
            // Calculate time to next waypoint
            double nextSegmentTime = 0.0;
            double nextDistance = 0.0;

            if (currentWaypoint + 1 < waypoints.count()) {
                nextSegmentTime = calculateSegmentTime(
                    waypoints[currentWaypoint],
                    waypoints[currentWaypoint + 1],
                    speed);
                nextDistance = Geospatial::GeoUtils::distanceBetween(
                    waypoints[currentWaypoint].coordinate(),
                    waypoints[currentWaypoint + 1].coordinate());
            }

            // Calculate RTH time from next position
            Models::GeospatialCoordinate nextPos = (currentWaypoint + 1 < waypoints.count()) ?
                waypoints[currentWaypoint + 1].coordinate() :
                waypoints[currentWaypoint].coordinate();

            double rthTime = calculateReturnTime(nextPos, homePoint, speed);

            // Check if we can do this waypoint and still return home
            if (segmentTime + nextSegmentTime + rthTime + 1.0 > availableTime) {
                // Can't do this waypoint - stop here
                break;
            }

            currentWaypoint++;
            waypointsThisBattery++;
            segmentTime += nextSegmentTime;
            distanceThisBattery += nextDistance;
        }

        // Fill in mission details
        mission.waypointEnd = currentWaypoint - 1;

        for (int i = mission.waypointStart; i <= mission.waypointEnd && i < waypoints.count(); ++i) {
            mission.waypoints.append(waypoints[i]);
        }

        mission.estimatedFlightTime = segmentTime + 1.0; // +1 for landing
        mission.estimatedDistance = distanceThisBattery;
        mission.landingPoint = homePoint;

        if (batteryNum < numBatteries) {
            mission.notes = QString("Land at home, swap battery %1 � %2, resume")
                .arg(batteryNum).arg(batteryNum + 1);
        } else {
            mission.notes = "Final battery - mission complete";
        }

        subMissions.append(mission);

        // If we've covered all waypoints, stop
        if (currentWaypoint >= waypoints.count()) {
            break;
        }
    }

    return subMissions;
}

int BatteryManager::findOptimalReturnPoint(
    const QList<Models::Waypoint>& waypoints,
    const Models::GeospatialCoordinate& homePoint,
    double maxFlightTime,
    double currentTime)
{
    // Find the furthest waypoint we can reach and still return home safely
    double remainingTime = maxFlightTime - currentTime;
    double speed = 10.0; // Default cruise speed

    for (int i = 0; i < waypoints.count(); ++i) {
        double rthTime = calculateReturnTime(waypoints[i].coordinate(), homePoint, speed);

        // Add 1 minute safety buffer
        if (rthTime + 1.0 > remainingTime) {
            // Return previous waypoint
            return qMax(0, i - 1);
        }
    }

    return waypoints.count() - 1;
}

double BatteryManager::calculateEnergyConsumption(
    double distance,
    double altitude,
    double speed,
    const BatteryProfile& profile)
{
    // Simplified energy model
    // Energy (Wh) = Power (W) � Time (h)
    // Power varies with speed, altitude, and drone characteristics

    double timeHours = (distance / speed) / 3600.0; // seconds to hours

    // Base power consumption
    double basePower = profile.hoverCurrentDraw * profile.nominalVoltage;

    // Speed factor (more speed = more power)
    double speedFactor = 1.0 + (speed / 15.0) * 0.3; // 30% increase at max speed

    // Altitude factor (higher altitude = slightly more power for stability)
    double altitudeFactor = 1.0 + (altitude / 120.0) * 0.1; // 10% increase at 120m

    double totalPower = basePower * speedFactor * altitudeFactor;
    double energy = totalPower * timeHours;

    return energy;
}

double BatteryManager::estimateCurrentDraw(
    double speed,
    double altitude,
    double windSpeed,
    const BatteryProfile& profile)
{
    // Start with base current (hover)
    double current = profile.hoverCurrentDraw;

    // Speed factor
    double speedRatio = speed / 15.0; // Normalize to max speed (~15 m/s)
    current += (profile.cruiseCurrentDraw - profile.hoverCurrentDraw) * speedRatio;

    // Wind resistance factor
    double windFactor = 1.0 + (windSpeed / 10.0) * 0.2; // 20% increase at 10 m/s wind
    current *= windFactor;

    // Altitude factor (thin air at high altitude)
    if (altitude > 100.0) {
        double altitudeFactor = 1.0 + ((altitude - 100.0) / 200.0) * 0.15;
        current *= altitudeFactor;
    }

    return current;
}

QString BatteryManager::getBatterySwapInstructions(
    const QList<SubMission>& subMissions)
{
    if (subMissions.isEmpty()) {
        return "No mission data available.";
    }

    if (subMissions.count() == 1) {
        return QString("Single battery mission\n"
                      "Estimated flight time: %1 minutes\n"
                      "Complete mission without battery swap")
            .arg(subMissions[0].estimatedFlightTime, 0, 'f', 1);
    }

    QString instructions;
    instructions += QString("Multi-battery mission - %1 batteries required\n\n")
        .arg(subMissions.count());

    for (int i = 0; i < subMissions.count(); ++i) {
        const auto& mission = subMissions[i];
        instructions += QString("Battery %1:\n").arg(mission.batteryNumber);
        instructions += QString("  - Waypoints: %1 to %2 (%3 total)\n")
            .arg(mission.waypointStart + 1)
            .arg(mission.waypointEnd + 1)
            .arg(mission.waypoints.count());
        instructions += QString("  - Flight time: ~%1 minutes\n")
            .arg(mission.estimatedFlightTime, 0, 'f', 1);
        instructions += QString("  - Distance: %1 m\n")
            .arg(mission.estimatedDistance, 0, 'f', 0);

        if (i < subMissions.count() - 1) {
            instructions += QString("  - After completion: Land, swap battery, resume\n\n");
        } else {
            instructions += QString("  - Mission complete after this battery\n\n");
        }
    }

    instructions += "\nPre-flight Checklist:\n";
    instructions += QString("  - Charge all %1 batteries to 100%%\n").arg(subMissions.count());
    instructions += "  - Have battery swap station at launch point\n";
    instructions += "  - Monitor battery percentage during flight\n";
    instructions += "  - Land immediately if low battery warning occurs\n";

    return instructions;
}

double BatteryManager::calculateSegmentTime(
    const Models::Waypoint& from,
    const Models::Waypoint& to,
    double speed)
{
    double distance = Geospatial::GeoUtils::distanceBetween(
        from.coordinate(), to.coordinate());

    if (speed <= 0.0) speed = 10.0; // Default speed

    // Time in seconds
    double timeSeconds = distance / speed;

    // Convert to minutes
    return timeSeconds / 60.0;
}

double BatteryManager::calculateReturnTime(
    const Models::GeospatialCoordinate& position,
    const Models::GeospatialCoordinate& home,
    double speed)
{
    double distance = Geospatial::GeoUtils::distanceBetween(position, home);

    if (speed <= 0.0) speed = 10.0;

    // Time in seconds, convert to minutes
    double timeSeconds = distance / speed;

    // Add landing time (30 seconds)
    timeSeconds += 30.0;

    return timeSeconds / 60.0;
}

} // namespace Core
} // namespace DroneMapper
