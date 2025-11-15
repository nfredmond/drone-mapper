#ifndef QUALITYESTIMATOR_H
#define QUALITYESTIMATOR_H

#include "FlightPlan.h"
#include "MissionParameters.h"
#include <QString>
#include <QList>

namespace DroneMapper {
namespace Photogrammetry {

/**
 * @brief Quality score categories
 */
enum class QualityLevel {
    Excellent,      // 90-100%
    Good,           // 75-89%
    Acceptable,     // 60-74%
    Poor,           // 40-59%
    Inadequate      // 0-39%
};

/**
 * @brief Quality assessment factors
 */
struct QualityFactors {
    double gsdScore;              // 0-100 (Ground Sampling Distance)
    double overlapScore;          // 0-100 (Image overlap)
    double coverageScore;         // 0-100 (Area coverage uniformity)
    double altitudeScore;         // 0-100 (Altitude consistency)
    double lightingScore;         // 0-100 (Lighting conditions)
    double cameraScore;           // 0-100 (Camera quality)
    double imageCountScore;       // 0-100 (Sufficient images)
    double geometryScore;         // 0-100 (Geometry/viewing angles)

    double getOverallScore() const;
    QString getWeakestFactor() const;
};

/**
 * @brief Reconstruction quality prediction
 */
struct QualityEstimate {
    double overallScore;           // 0-100
    QualityLevel level;
    QualityFactors factors;

    // Predictions
    double expectedAccuracy;       // Centimeters
    double expectedDensity;        // Points per square meter
    double expectedCompleteness;   // Percentage of area reconstructed
    double meshQuality;            // 0-100 mesh quality score

    // Recommendations
    QStringList strengths;
    QStringList weaknesses;
    QStringList recommendations;

    QString getSummary() const;
    QString getDetailedReport() const;
};

/**
 * @brief Image analysis for reconstruction
 */
struct ImageAnalysis {
    int totalImages;
    int usableImages;
    int blurryImages;
    int overexposedImages;
    int underexposedImages;
    double averageSharpness;       // 0-100
    double averageExposure;        // 0-100
    QList<int> imageGaps;          // Indices where coverage gaps exist

    QString getReport() const;
};

/**
 * @brief 3D reconstruction complexity
 */
enum class ReconstructionComplexity {
    Simple,         // Flat terrain, uniform texture
    Moderate,       // Some elevation, mixed terrain
    Complex,        // Varied elevation, complex features
    VeryComplex     // Extreme terrain, challenging conditions
};

/**
 * @brief Estimates photogrammetry reconstruction quality
 *
 * Features:
 * - GSD-based quality assessment
 * - Overlap sufficiency checking
 * - Coverage uniformity analysis
 * - Lighting condition evaluation
 * - Camera quality scoring
 * - Geometry optimization
 * - Accuracy prediction
 * - Density estimation
 * - Completeness prediction
 *
 * Quality factors:
 * - Ground Sampling Distance (GSD)
 * - Image overlap (front/side)
 * - Number of images
 * - Flight altitude consistency
 * - Viewing angle diversity
 * - Camera sensor quality
 * - Lighting conditions
 * - Texture availability
 */
class QualityEstimator {
public:
    QualityEstimator();

    /**
     * @brief Estimate reconstruction quality
     * @param plan Flight plan
     * @param params Mission parameters
     * @return Quality estimate
     */
    static QualityEstimate estimateQuality(
        const Models::FlightPlan& plan,
        const Models::MissionParameters& params);

    /**
     * @brief Calculate GSD quality score
     * @param gsd Ground Sampling Distance (cm/pixel)
     * @param targetGSD Target GSD for application
     * @return Score 0-100
     */
    static double calculateGSDScore(
        double gsd,
        double targetGSD = 2.0);

    /**
     * @brief Calculate overlap quality score
     * @param frontOverlap Front overlap percentage
     * @param sideOverlap Side overlap percentage
     * @return Score 0-100
     */
    static double calculateOverlapScore(
        double frontOverlap,
        double sideOverlap);

    /**
     * @brief Calculate coverage quality score
     * @param plan Flight plan
     * @return Score 0-100 (uniformity of coverage)
     */
    static double calculateCoverageScore(
        const Models::FlightPlan& plan);

    /**
     * @brief Calculate altitude consistency score
     * @param plan Flight plan
     * @return Score 0-100
     */
    static double calculateAltitudeScore(
        const Models::FlightPlan& plan);

    /**
     * @brief Calculate camera quality score
     * @param cameraModel Camera model
     * @return Score 0-100
     */
    static double calculateCameraScore(
        Models::MissionParameters::CameraModel cameraModel);

    /**
     * @brief Calculate image count score
     * @param imageCount Number of images
     * @param surveyAreaSqm Survey area in square meters
     * @return Score 0-100
     */
    static double calculateImageCountScore(
        int imageCount,
        double surveyAreaSqm);

    /**
     * @brief Predict reconstruction accuracy
     * @param gsd Ground Sampling Distance
     * @param overlap Average overlap percentage
     * @return Expected accuracy in centimeters
     */
    static double predictAccuracy(
        double gsd,
        double overlap);

    /**
     * @brief Predict point cloud density
     * @param gsd Ground Sampling Distance
     * @param imageCount Number of images
     * @param surveyAreaSqm Survey area
     * @return Points per square meter
     */
    static double predictDensity(
        double gsd,
        int imageCount,
        double surveyAreaSqm);

    /**
     * @brief Predict reconstruction completeness
     * @param overlapScore Overlap quality score
     * @param coverageScore Coverage quality score
     * @return Percentage of area expected to be reconstructed
     */
    static double predictCompleteness(
        double overlapScore,
        double coverageScore);

    /**
     * @brief Generate recommendations for improvement
     * @param estimate Quality estimate
     * @return List of actionable recommendations
     */
    static QStringList generateRecommendations(
        const QualityEstimate& estimate);

    /**
     * @brief Determine quality level from score
     * @param score Overall quality score (0-100)
     * @return Quality level
     */
    static QualityLevel getQualityLevel(double score);

    /**
     * @brief Get quality level name
     * @param level Quality level
     * @return Human-readable name
     */
    static QString getQualityLevelName(QualityLevel level);

    /**
     * @brief Assess reconstruction complexity
     * @param plan Flight plan
     * @param params Mission parameters
     * @return Complexity level
     */
    static ReconstructionComplexity assessComplexity(
        const Models::FlightPlan& plan,
        const Models::MissionParameters& params);

    /**
     * @brief Calculate optimal GSD for target accuracy
     * @param targetAccuracyCm Target accuracy in centimeters
     * @return Recommended GSD in cm/pixel
     */
    static double calculateOptimalGSD(double targetAccuracyCm);

    /**
     * @brief Calculate optimal overlap for target completeness
     * @param targetCompleteness Target completeness percentage
     * @return Recommended overlap percentage
     */
    static double calculateOptimalOverlap(double targetCompleteness);

private:
    static double normalizeScore(
        double value,
        double min,
        double max,
        bool inverse = false);

    static QStringList identifyStrengths(const QualityFactors& factors);
    static QStringList identifyWeaknesses(const QualityFactors& factors);
};

} // namespace Photogrammetry
} // namespace DroneMapper

#endif // QUALITYESTIMATOR_H
