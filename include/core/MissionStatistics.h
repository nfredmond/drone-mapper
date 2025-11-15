#ifndef MISSIONSTATISTICS_H
#define MISSIONSTATISTICS_H

#include "FlightPlan.h"
#include "MissionParameters.h"
#include "BatteryManager.h"
#include <QString>
#include <QList>

namespace DroneMapper {
namespace Core {

/**
 * @brief Comprehensive mission statistics
 */
struct MissionStats {
    // Basic metrics
    int waypointCount;
    double totalDistance;            // Meters
    double averageSpeed;             // m/s
    double estimatedFlightTime;      // Minutes
    double estimatedBatteryUsage;    // Percentage
    int batteriesRequired;

    // Coverage metrics
    double surveyArea;               // Square meters
    double effectiveCoverage;        // Square meters (accounting for overlap)
    double overlapPercentage;        // Front/side overlap average
    int estimatedPhotoCount;
    double groundSamplingDistance;   // cm/pixel

    // Altitude metrics
    double minAltitude;              // Meters AGL
    double maxAltitude;              // Meters AGL
    double avgAltitude;              // Meters AGL
    double altitudeVariance;         // Standard deviation

    // Efficiency metrics
    double flightEfficiency;         // 0-100% (straight-line vs actual distance)
    int turnCount;
    double avgTurnAngle;             // Degrees
    double pathOptimization;         // 0-100% (how optimized the path is)

    // Quality metrics
    double imageQualityScore;        // 0-100% (based on GSD, overlap, altitude)
    double coverageQuality;          // 0-100% (uniformity of coverage)
    double weatherSuitability;       // 0-100% (if weather data available)

    // Cost/resource metrics
    double estimatedCost;            // USD (if configured)
    double dataSizeGB;               // Estimated output data size
    int storageCardsNeeded;          // Number of SD cards
    double processingTimeHours;      // Estimated photogrammetry processing

    QString getSummary() const;
    QString getDetailedReport() const;
};

/**
 * @brief Comparison between two missions
 */
struct MissionComparison {
    QString mission1Name;
    QString mission2Name;
    MissionStats stats1;
    MissionStats stats2;

    double distanceDiff;             // Meters
    double timeDiff;                 // Minutes
    double areaDiff;                 // Square meters
    double costDiff;                 // USD
    double efficiencyDiff;           // Percentage points

    QString betterMission;           // Which mission is more efficient
    QString getComparisonReport() const;
};

/**
 * @brief Time-series mission trends
 */
struct MissionTrends {
    QList<QString> missionNames;
    QList<double> distances;
    QList<double> times;
    QList<double> areas;
    QList<double> costs;

    double avgDistance() const;
    double avgTime() const;
    double avgArea() const;
    QString getTrendReport() const;
};

/**
 * @brief Analyzes flight plan statistics and metrics
 *
 * Features:
 * - Comprehensive mission metrics
 * - Coverage area calculation
 * - Photo count estimation
 * - Cost analysis
 * - Efficiency scoring
 * - Quality assessment
 * - Mission comparison
 * - Trend analysis
 * - Report generation
 *
 * Calculations include:
 * - Ground Sampling Distance (GSD)
 * - Image overlap percentages
 * - Survey area coverage
 * - Flight efficiency
 * - Data storage requirements
 * - Processing time estimates
 */
class MissionStatistics {
public:
    MissionStatistics();

    /**
     * @brief Calculate comprehensive statistics for flight plan
     * @param plan Flight plan to analyze
     * @param params Mission parameters
     * @return Complete mission statistics
     */
    static MissionStats analyze(
        const Models::FlightPlan& plan,
        const Models::MissionParameters& params);

    /**
     * @brief Calculate survey area covered by mission
     * @param plan Flight plan
     * @param params Mission parameters
     * @return Area in square meters
     */
    static double calculateSurveyArea(
        const Models::FlightPlan& plan,
        const Models::MissionParameters& params);

    /**
     * @brief Estimate number of photos to be captured
     * @param plan Flight plan
     * @param params Mission parameters
     * @return Estimated photo count
     */
    static int estimatePhotoCount(
        const Models::FlightPlan& plan,
        const Models::MissionParameters& params);

    /**
     * @brief Calculate Ground Sampling Distance
     * @param altitude Flight altitude (meters)
     * @param focalLength Camera focal length (mm)
     * @param sensorWidth Sensor width (mm)
     * @param imageWidth Image width (pixels)
     * @return GSD in cm/pixel
     */
    static double calculateGSD(
        double altitude,
        double focalLength,
        double sensorWidth,
        int imageWidth);

    /**
     * @brief Calculate flight efficiency score
     * @param plan Flight plan
     * @return Efficiency percentage (0-100)
     */
    static double calculateFlightEfficiency(
        const Models::FlightPlan& plan);

    /**
     * @brief Calculate image quality score
     * @param params Mission parameters
     * @param gsd Ground sampling distance
     * @return Quality score (0-100)
     */
    static double calculateImageQualityScore(
        const Models::MissionParameters& params,
        double gsd);

    /**
     * @brief Estimate data storage requirements
     * @param photoCount Number of photos
     * @param imageWidth Image width (pixels)
     * @param imageHeight Image height (pixels)
     * @return Storage size in GB
     */
    static double estimateDataSize(
        int photoCount,
        int imageWidth,
        int imageHeight);

    /**
     * @brief Estimate photogrammetry processing time
     * @param photoCount Number of images
     * @param imageResolution Total megapixels
     * @return Processing time in hours
     */
    static double estimateProcessingTime(
        int photoCount,
        double imageResolution);

    /**
     * @brief Calculate mission cost
     * @param stats Mission statistics
     * @param costPerHour Operator cost per hour
     * @param costPerBattery Cost per battery cycle
     * @return Total estimated cost
     */
    static double calculateMissionCost(
        const MissionStats& stats,
        double costPerHour = 75.0,      // Default $75/hr
        double costPerBattery = 2.0);   // Default $2/battery

    /**
     * @brief Compare two missions
     * @param plan1 First flight plan
     * @param params1 First mission parameters
     * @param plan2 Second flight plan
     * @param params2 Second mission parameters
     * @return Comparison results
     */
    static MissionComparison compareMissions(
        const Models::FlightPlan& plan1,
        const Models::MissionParameters& params1,
        const Models::FlightPlan& plan2,
        const Models::MissionParameters& params2);

    /**
     * @brief Generate printable statistics report
     * @param stats Mission statistics
     * @return Formatted report string
     */
    static QString generateReport(const MissionStats& stats);

    /**
     * @brief Export statistics to CSV format
     * @param stats Mission statistics
     * @return CSV formatted string
     */
    static QString exportToCSV(const MissionStats& stats);

private:
    static double calculateEffectiveCoverage(
        double surveyArea,
        double frontOverlap,
        double sideOverlap);

    static int calculateTurnCount(
        const QList<Models::Waypoint>& waypoints);

    static double calculateAvgTurnAngle(
        const QList<Models::Waypoint>& waypoints);

    static double calculateCoverageQuality(
        const Models::FlightPlan& plan,
        const Models::MissionParameters& params);

    static void calculateCameraSpecs(
        Models::MissionParameters::CameraModel model,
        double& focalLength,
        double& sensorWidth,
        int& imageWidth,
        int& imageHeight);
};

} // namespace Core
} // namespace DroneMapper

#endif // MISSIONSTATISTICS_H
