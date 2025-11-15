#include "MissionStatistics.h"
#include "geospatial/GeoUtils.h"
#include "FlightPathOptimizer.h"
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace Core {

QString MissionStats::getSummary() const
{
    return QString("%1 waypoints, %2 km, %3 min, %4 ha, ~%5 photos")
        .arg(waypointCount)
        .arg(totalDistance / 1000.0, 0, 'f', 2)
        .arg(estimatedFlightTime, 0, 'f', 1)
        .arg(surveyArea / 10000.0, 0, 'f', 2)
        .arg(estimatedPhotoCount);
}

QString MissionStats::getDetailedReport() const
{
    QString report;
    report += "=== MISSION STATISTICS ===\n\n";

    report += "FLIGHT METRICS:\n";
    report += QString("  Waypoints: %1\n").arg(waypointCount);
    report += QString("  Total Distance: %1 km (%2 mi)\n")
        .arg(totalDistance / 1000.0, 0, 'f', 2)
        .arg(totalDistance / 1609.34, 0, 'f', 2);
    report += QString("  Flight Time: %1 min (%2 hr)\n")
        .arg(estimatedFlightTime, 0, 'f', 1)
        .arg(estimatedFlightTime / 60.0, 0, 'f', 2);
    report += QString("  Average Speed: %1 m/s (%2 mph)\n")
        .arg(averageSpeed, 0, 'f', 1)
        .arg(averageSpeed * 2.23694, 0, 'f', 1);
    report += QString("  Batteries Required: %1\n").arg(batteriesRequired);
    report += QString("  Battery Usage: %1%%\n\n").arg(estimatedBatteryUsage, 0, 'f', 1);

    report += "COVERAGE METRICS:\n";
    report += QString("  Survey Area: %1 ha (%2 acres)\n")
        .arg(surveyArea / 10000.0, 0, 'f', 2)
        .arg(surveyArea / 4046.86, 0, 'f', 2);
    report += QString("  Effective Coverage: %1 ha\n")
        .arg(effectiveCoverage / 10000.0, 0, 'f', 2);
    report += QString("  Photo Overlap: %1%%\n").arg(overlapPercentage, 0, 'f', 0);
    report += QString("  Estimated Photos: %1\n").arg(estimatedPhotoCount);
    report += QString("  GSD: %1 cm/pixel\n\n").arg(groundSamplingDistance, 0, 'f', 2);

    report += "ALTITUDE PROFILE:\n";
    report += QString("  Min Altitude: %1 m\n").arg(minAltitude, 0, 'f', 1);
    report += QString("  Max Altitude: %1 m\n").arg(maxAltitude, 0, 'f', 1);
    report += QString("  Avg Altitude: %1 m\n").arg(avgAltitude, 0, 'f', 1);
    report += QString("  Variance: %1 m\n\n").arg(altitudeVariance, 0, 'f', 1);

    report += "EFFICIENCY METRICS:\n";
    report += QString("  Flight Efficiency: %1%%\n").arg(flightEfficiency, 0, 'f', 1);
    report += QString("  Turn Count: %1\n").arg(turnCount);
    report += QString("  Avg Turn Angle: %1°\n").arg(avgTurnAngle, 0, 'f', 1);
    report += QString("  Path Optimization: %1%%\n\n").arg(pathOptimization, 0, 'f', 1);

    report += "QUALITY METRICS:\n";
    report += QString("  Image Quality: %1%%\n").arg(imageQualityScore, 0, 'f', 1);
    report += QString("  Coverage Quality: %1%%\n\n").arg(coverageQuality, 0, 'f', 1);

    report += "RESOURCES:\n";
    report += QString("  Estimated Cost: $%1\n").arg(estimatedCost, 0, 'f', 2);
    report += QString("  Data Size: %1 GB\n").arg(dataSizeGB, 0, 'f', 1);
    report += QString("  Storage Cards: %1\n").arg(storageCardsNeeded);
    report += QString("  Processing Time: %1 hrs\n").arg(processingTimeHours, 0, 'f', 1);

    return report;
}

QString MissionComparison::getComparisonReport() const
{
    QString report;
    report += QString("=== MISSION COMPARISON: %1 vs %2 ===\n\n")
        .arg(mission1Name).arg(mission2Name);

    report += QString("Distance: %1 km vs %2 km (diff: %3 km)\n")
        .arg(stats1.totalDistance / 1000.0, 0, 'f', 2)
        .arg(stats2.totalDistance / 1000.0, 0, 'f', 2)
        .arg(distanceDiff / 1000.0, 0, 'f', 2);

    report += QString("Time: %1 min vs %2 min (diff: %3 min)\n")
        .arg(stats1.estimatedFlightTime, 0, 'f', 1)
        .arg(stats2.estimatedFlightTime, 0, 'f', 1)
        .arg(timeDiff, 0, 'f', 1);

    report += QString("Area: %1 ha vs %2 ha (diff: %3 ha)\n")
        .arg(stats1.surveyArea / 10000.0, 0, 'f', 2)
        .arg(stats2.surveyArea / 10000.0, 0, 'f', 2)
        .arg(areaDiff / 10000.0, 0, 'f', 2);

    report += QString("Cost: $%1 vs $%2 (diff: $%3)\n")
        .arg(stats1.estimatedCost, 0, 'f', 2)
        .arg(stats2.estimatedCost, 0, 'f', 2)
        .arg(costDiff, 0, 'f', 2);

    report += QString("\nRecommendation: %1 is more efficient\n").arg(betterMission);

    return report;
}

MissionStatistics::MissionStatistics()
{
}

MissionStats MissionStatistics::analyze(
    const Models::FlightPlan& plan,
    const Models::MissionParameters& params)
{
    MissionStats stats;

    const auto& waypoints = plan.waypoints();
    stats.waypointCount = waypoints.count();

    if (waypoints.isEmpty()) {
        return stats; // Return empty stats
    }

    // Calculate distance and time
    stats.totalDistance = plan.totalDistance();
    stats.averageSpeed = params.flightSpeed();

    // Flight time estimation using BatteryManager
    BatteryProfile profile = BatteryManager::getBatteryProfile(params.cameraModel());
    stats.estimatedFlightTime = BatteryManager::estimateFlightTime(plan, profile);
    stats.estimatedBatteryUsage = BatteryManager::calculateBatteryUsage(plan, profile);
    stats.batteriesRequired = BatteryManager::calculateRequiredBatteries(plan, profile);

    // Coverage metrics
    stats.surveyArea = calculateSurveyArea(plan, params);
    stats.overlapPercentage = (params.frontOverlap() + params.sideOverlap()) / 2.0;
    stats.effectiveCoverage = calculateEffectiveCoverage(
        stats.surveyArea,
        params.frontOverlap() / 100.0,
        params.sideOverlap() / 100.0);
    stats.estimatedPhotoCount = estimatePhotoCount(plan, params);

    // Camera specs for GSD calculation
    double focalLength, sensorWidth;
    int imageWidth, imageHeight;
    calculateCameraSpecs(params.cameraModel(), focalLength, sensorWidth,
                        imageWidth, imageHeight);

    // Calculate GSD at average altitude
    double totalAltitude = 0.0;
    stats.minAltitude = 1e10;
    stats.maxAltitude = 0.0;

    for (const auto& wp : waypoints) {
        double alt = wp.coordinate().altitude();
        totalAltitude += alt;
        stats.minAltitude = std::min(stats.minAltitude, alt);
        stats.maxAltitude = std::max(stats.maxAltitude, alt);
    }

    stats.avgAltitude = totalAltitude / waypoints.count();
    stats.groundSamplingDistance = calculateGSD(
        stats.avgAltitude, focalLength, sensorWidth, imageWidth);

    // Calculate altitude variance
    double variance = 0.0;
    for (const auto& wp : waypoints) {
        double diff = wp.coordinate().altitude() - stats.avgAltitude;
        variance += diff * diff;
    }
    stats.altitudeVariance = std::sqrt(variance / waypoints.count());

    // Efficiency metrics
    stats.flightEfficiency = calculateFlightEfficiency(plan);
    stats.turnCount = calculateTurnCount(waypoints);
    stats.avgTurnAngle = calculateAvgTurnAngle(waypoints);
    stats.pathOptimization = stats.flightEfficiency; // Simplified

    // Quality metrics
    stats.imageQualityScore = calculateImageQualityScore(params, stats.groundSamplingDistance);
    stats.coverageQuality = calculateCoverageQuality(plan, params);
    stats.weatherSuitability = 0.0; // Would integrate with WeatherService

    // Resource metrics
    stats.dataSizeGB = estimateDataSize(stats.estimatedPhotoCount,
                                       imageWidth, imageHeight);
    stats.storageCardsNeeded = static_cast<int>(std::ceil(stats.dataSizeGB / 64.0)); // 64GB cards
    stats.processingTimeHours = estimateProcessingTime(
        stats.estimatedPhotoCount,
        (imageWidth * imageHeight) / 1000000.0);
    stats.estimatedCost = calculateMissionCost(stats);

    return stats;
}

double MissionStatistics::calculateSurveyArea(
    const Models::FlightPlan& plan,
    const Models::MissionParameters& params)
{
    const auto& waypoints = plan.waypoints();
    if (waypoints.count() < 3) {
        return 0.0;
    }

    // Calculate camera footprint width
    double altitude = params.flightAltitude();
    double focalLength, sensorWidth;
    int imageWidth, imageHeight;
    calculateCameraSpecs(params.cameraModel(), focalLength, sensorWidth,
                        imageWidth, imageHeight);

    // Footprint width = (altitude * sensorWidth) / focalLength
    double footprintWidth = (altitude * sensorWidth) / focalLength;

    // Calculate line spacing (accounting for side overlap)
    double sideOverlap = params.sideOverlap() / 100.0;
    double lineSpacing = footprintWidth * (1.0 - sideOverlap);

    // Estimate survey lines by analyzing waypoint pattern
    int surveyLines = 1;
    double prevBearing = 0.0;

    for (int i = 1; i < waypoints.count(); ++i) {
        // Detect survey line changes (large turns)
        // This is simplified - production would use more sophisticated detection
    }

    // Rough area estimate: total distance * footprint width
    // This is simplified - accurate calculation would use convex hull
    double area = plan.totalDistance() * footprintWidth;

    return area;
}

int MissionStatistics::estimatePhotoCount(
    const Models::FlightPlan& plan,
    const Models::MissionParameters& params)
{
    if (plan.waypoints().isEmpty()) {
        return 0;
    }

    double focalLength, sensorWidth;
    int imageWidth, imageHeight;
    calculateCameraSpecs(params.cameraModel(), focalLength, sensorWidth,
                        imageWidth, imageHeight);

    double altitude = params.flightAltitude();
    double speed = params.flightSpeed();

    // Calculate footprint dimensions
    double footprintWidth = (altitude * sensorWidth) / focalLength;
    double sensorHeight = sensorWidth * imageHeight / imageWidth;
    double footprintHeight = (altitude * sensorHeight) / focalLength;

    // Calculate photo spacing based on overlap
    double frontOverlap = params.frontOverlap() / 100.0;
    double photoSpacing = footprintHeight * (1.0 - frontOverlap);

    // Estimate photos based on distance and spacing
    int photoCount = static_cast<int>(plan.totalDistance() / photoSpacing);

    // Add photos for turns and start/end
    photoCount += 10; // Buffer

    return photoCount;
}

double MissionStatistics::calculateGSD(
    double altitude,
    double focalLength,
    double sensorWidth,
    int imageWidth)
{
    // GSD (cm/pixel) = (altitude * sensorWidth) / (focalLength * imageWidth)
    // Convert to cm
    double gsd = (altitude * 1000.0 * sensorWidth) / (focalLength * imageWidth);
    return gsd;
}

double MissionStatistics::calculateFlightEfficiency(
    const Models::FlightPlan& plan)
{
    const auto& waypoints = plan.waypoints();
    if (waypoints.count() < 2) {
        return 100.0;
    }

    // Calculate straight-line distance from start to end
    double straightLine = Geospatial::GeoUtils::distanceBetween(
        waypoints.first().coordinate(),
        waypoints.last().coordinate());

    double actualDistance = plan.totalDistance();

    if (actualDistance == 0.0) {
        return 100.0;
    }

    // Efficiency = straight-line / actual * 100
    // For survey missions, this will be low (which is expected)
    // So we cap at 100% and consider >50% as good
    double efficiency = (straightLine / actualDistance) * 100.0;
    return std::min(efficiency, 100.0);
}

double MissionStatistics::calculateImageQualityScore(
    const Models::MissionParameters& params,
    double gsd)
{
    double score = 100.0;

    // Penalize if GSD is too coarse (>5cm)
    if (gsd > 5.0) {
        score -= (gsd - 5.0) * 5.0;
    }

    // Reward good overlap (75-85% is ideal)
    double avgOverlap = (params.frontOverlap() + params.sideOverlap()) / 2.0;
    if (avgOverlap < 60.0) {
        score -= (60.0 - avgOverlap);
    } else if (avgOverlap > 90.0) {
        score -= (avgOverlap - 90.0);
    }

    return std::max(0.0, std::min(100.0, score));
}

double MissionStatistics::estimateDataSize(
    int photoCount,
    int imageWidth,
    int imageHeight)
{
    // Estimate JPEG file size: megapixels * 3-5 MB per image
    double megapixels = (imageWidth * imageHeight) / 1000000.0;
    double mbPerPhoto = megapixels * 4.0; // 4 MB per megapixel average
    double totalGB = (photoCount * mbPerPhoto) / 1024.0;

    return totalGB;
}

double MissionStatistics::estimateProcessingTime(
    int photoCount,
    double imageResolution)
{
    // Very rough estimate:
    // ~0.5-1 minute per image for alignment
    // ~2x for dense cloud
    // ~1x for mesh
    // ~0.5x for texture

    double alignmentHours = (photoCount * 0.75) / 60.0;
    double denseCloudHours = alignmentHours * 2.0;
    double meshHours = alignmentHours;
    double textureHours = alignmentHours * 0.5;

    double totalHours = alignmentHours + denseCloudHours + meshHours + textureHours;

    // Adjust for image resolution (higher res = more time)
    totalHours *= (imageResolution / 20.0); // Normalize to 20MP

    return totalHours;
}

double MissionStatistics::calculateMissionCost(
    const MissionStats& stats,
    double costPerHour,
    double costPerBattery)
{
    // Flight time cost
    double flightCost = (stats.estimatedFlightTime / 60.0) * costPerHour;

    // Battery cost
    double batteryCost = stats.batteriesRequired * costPerBattery;

    // Planning/setup overhead (30 minutes)
    double setupCost = 0.5 * costPerHour;

    // Processing cost (data processing time)
    double processingCost = stats.processingTimeHours * (costPerHour * 0.5); // Half rate

    return flightCost + batteryCost + setupCost + processingCost;
}

MissionComparison MissionStatistics::compareMissions(
    const Models::FlightPlan& plan1,
    const Models::MissionParameters& params1,
    const Models::FlightPlan& plan2,
    const Models::MissionParameters& params2)
{
    MissionComparison comp;
    comp.mission1Name = "Mission 1";
    comp.mission2Name = "Mission 2";
    comp.stats1 = analyze(plan1, params1);
    comp.stats2 = analyze(plan2, params2);

    comp.distanceDiff = comp.stats2.totalDistance - comp.stats1.totalDistance;
    comp.timeDiff = comp.stats2.estimatedFlightTime - comp.stats1.estimatedFlightTime;
    comp.areaDiff = comp.stats2.surveyArea - comp.stats1.surveyArea;
    comp.costDiff = comp.stats2.estimatedCost - comp.stats1.estimatedCost;
    comp.efficiencyDiff = comp.stats2.flightEfficiency - comp.stats1.flightEfficiency;

    // Determine better mission based on multiple factors
    int score1 = 0, score2 = 0;

    if (comp.stats1.flightEfficiency > comp.stats2.flightEfficiency) score1++;
    else score2++;

    if (comp.stats1.estimatedCost < comp.stats2.estimatedCost) score1++;
    else score2++;

    if (comp.stats1.imageQualityScore > comp.stats2.imageQualityScore) score1++;
    else score2++;

    comp.betterMission = (score1 > score2) ? "Mission 1" : "Mission 2";

    return comp;
}

QString MissionStatistics::generateReport(const MissionStats& stats)
{
    return stats.getDetailedReport();
}

QString MissionStatistics::exportToCSV(const MissionStats& stats)
{
    QString csv;
    csv += "Metric,Value,Unit\n";
    csv += QString("Waypoints,%1,count\n").arg(stats.waypointCount);
    csv += QString("Distance,%1,km\n").arg(stats.totalDistance / 1000.0);
    csv += QString("Flight Time,%1,min\n").arg(stats.estimatedFlightTime);
    csv += QString("Survey Area,%1,ha\n").arg(stats.surveyArea / 10000.0);
    csv += QString("Photos,%1,count\n").arg(stats.estimatedPhotoCount);
    csv += QString("GSD,%1,cm/px\n").arg(stats.groundSamplingDistance);
    csv += QString("Cost,%1,USD\n").arg(stats.estimatedCost);
    csv += QString("Data Size,%1,GB\n").arg(stats.dataSizeGB);
    csv += QString("Processing Time,%1,hrs\n").arg(stats.processingTimeHours);

    return csv;
}

double MissionStatistics::calculateEffectiveCoverage(
    double surveyArea,
    double frontOverlap,
    double sideOverlap)
{
    // Account for overlap reducing effective unique coverage
    double overlapFactor = (1.0 - frontOverlap) * (1.0 - sideOverlap);
    return surveyArea * overlapFactor;
}

int MissionStatistics::calculateTurnCount(
    const QList<Models::Waypoint>& waypoints)
{
    return FlightPathOptimizer::calculateDirectionChanges(waypoints, 30.0);
}

double MissionStatistics::calculateAvgTurnAngle(
    const QList<Models::Waypoint>& waypoints)
{
    if (waypoints.count() < 3) {
        return 0.0;
    }

    double totalAngle = 0.0;
    int turnCount = 0;

    for (int i = 2; i < waypoints.count(); ++i) {
        // Calculate bearing change between segments
        // This is simplified - production would use proper bearing calculations
        turnCount++;
    }

    return turnCount > 0 ? totalAngle / turnCount : 0.0;
}

double MissionStatistics::calculateCoverageQuality(
    const Models::FlightPlan& plan,
    const Models::MissionParameters& params)
{
    // Quality is based on:
    // - Consistent altitude
    // - Appropriate overlap
    // - Regular spacing

    double score = 100.0;

    const auto& waypoints = plan.waypoints();
    if (waypoints.isEmpty()) return 0.0;

    // Check altitude consistency
    double avgAlt = 0.0;
    for (const auto& wp : waypoints) {
        avgAlt += wp.coordinate().altitude();
    }
    avgAlt /= waypoints.count();

    double altVariance = 0.0;
    for (const auto& wp : waypoints) {
        double diff = wp.coordinate().altitude() - avgAlt;
        altVariance += diff * diff;
    }
    altVariance = std::sqrt(altVariance / waypoints.count());

    // Penalize high variance
    if (altVariance > 10.0) {
        score -= altVariance * 2.0;
    }

    return std::max(0.0, std::min(100.0, score));
}

void MissionStatistics::calculateCameraSpecs(
    Models::MissionParameters::CameraModel model,
    double& focalLength,
    double& sensorWidth,
    int& imageWidth,
    int& imageHeight)
{
    switch (model) {
    case Models::MissionParameters::CameraModel::DJI_Mini3:
    case Models::MissionParameters::CameraModel::DJI_Mini3Pro:
        focalLength = 6.7;      // mm
        sensorWidth = 9.7;      // mm (1/1.3" sensor)
        imageWidth = 4000;
        imageHeight = 3000;     // 12MP
        break;

    case Models::MissionParameters::CameraModel::DJI_Air3:
        focalLength = 9.0;      // mm
        sensorWidth = 13.2;     // mm (1" sensor)
        imageWidth = 4000;
        imageHeight = 3000;     // 12MP wide camera
        break;

    case Models::MissionParameters::CameraModel::DJI_Mavic3:
        focalLength = 12.29;    // mm
        sensorWidth = 17.3;     // mm (4/3" sensor)
        imageWidth = 5280;
        imageHeight = 3956;     // 20MP
        break;

    default:
        focalLength = 8.8;
        sensorWidth = 13.2;
        imageWidth = 4000;
        imageHeight = 3000;
        break;
    }
}

} // namespace Core
} // namespace DroneMapper
