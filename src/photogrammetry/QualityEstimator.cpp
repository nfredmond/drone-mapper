#include "QualityEstimator.h"
#include "core/MissionStatistics.h"
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace Photogrammetry {

double QualityFactors::getOverallScore() const
{
    // Weighted average of all factors
    double total = 0.0;
    double weights = 0.0;

    // GSD and overlap are most important
    total += gsdScore * 2.0;
    weights += 2.0;

    total += overlapScore * 2.0;
    weights += 2.0;

    // Other factors
    total += coverageScore * 1.5;
    weights += 1.5;

    total += altitudeScore * 1.0;
    weights += 1.0;

    total += cameraScore * 1.0;
    weights += 1.0;

    total += imageCountScore * 1.5;
    weights += 1.5;

    total += geometryScore * 1.0;
    weights += 1.0;

    total += lightingScore * 0.5;
    weights += 0.5;

    return total / weights;
}

QString QualityFactors::getWeakestFactor() const
{
    double minScore = 100.0;
    QString weakest = "None";

    if (gsdScore < minScore) { minScore = gsdScore; weakest = "GSD"; }
    if (overlapScore < minScore) { minScore = overlapScore; weakest = "Overlap"; }
    if (coverageScore < minScore) { minScore = coverageScore; weakest = "Coverage"; }
    if (altitudeScore < minScore) { minScore = altitudeScore; weakest = "Altitude"; }
    if (cameraScore < minScore) { minScore = cameraScore; weakest = "Camera"; }
    if (imageCountScore < minScore) { minScore = imageCountScore; weakest = "Image Count"; }
    if (geometryScore < minScore) { minScore = geometryScore; weakest = "Geometry"; }
    if (lightingScore < minScore) { minScore = lightingScore; weakest = "Lighting"; }

    return weakest;
}

QString QualityEstimate::getSummary() const
{
    return QString("%1 quality (%2%) - Expected accuracy: %3 cm")
        .arg(QualityEstimator::getQualityLevelName(level))
        .arg(overallScore, 0, 'f', 1)
        .arg(expectedAccuracy, 0, 'f', 1);
}

QString QualityEstimate::getDetailedReport() const
{
    QString report;
    report += "=== PHOTOGRAMMETRY QUALITY ESTIMATE ===\n\n";

    report += QString("Overall Quality: %1 (%2%)\n\n")
        .arg(QualityEstimator::getQualityLevelName(level))
        .arg(overallScore, 0, 'f', 1);

    report += "QUALITY FACTORS:\n";
    report += QString("  GSD:         %1%\n").arg(factors.gsdScore, 0, 'f', 1);
    report += QString("  Overlap:     %1%\n").arg(factors.overlapScore, 0, 'f', 1);
    report += QString("  Coverage:    %1%\n").arg(factors.coverageScore, 0, 'f', 1);
    report += QString("  Altitude:    %1%\n").arg(factors.altitudeScore, 0, 'f', 1);
    report += QString("  Camera:      %1%\n").arg(factors.cameraScore, 0, 'f', 1);
    report += QString("  Image Count: %1%\n").arg(factors.imageCountScore, 0, 'f', 1);
    report += QString("  Geometry:    %1%\n").arg(factors.geometryScore, 0, 'f', 1);
    report += QString("  Lighting:    %1%\n\n").arg(factors.lightingScore, 0, 'f', 1);

    report += "PREDICTIONS:\n";
    report += QString("  Expected Accuracy:    %1 cm\n").arg(expectedAccuracy, 0, 'f', 1);
    report += QString("  Point Cloud Density:  %1 pts/m²\n").arg(expectedDensity, 0, 'f', 0);
    report += QString("  Completeness:         %1%\n").arg(expectedCompleteness, 0, 'f', 1);
    report += QString("  Mesh Quality:         %1%\n\n").arg(meshQuality, 0, 'f', 1);

    if (!strengths.isEmpty()) {
        report += "STRENGTHS:\n";
        for (const auto& s : strengths) {
            report += QString("  + %1\n").arg(s);
        }
        report += "\n";
    }

    if (!weaknesses.isEmpty()) {
        report += "WEAKNESSES:\n";
        for (const auto& w : weaknesses) {
            report += QString("  - %1\n").arg(w);
        }
        report += "\n";
    }

    if (!recommendations.isEmpty()) {
        report += "RECOMMENDATIONS:\n";
        for (const auto& r : recommendations) {
            report += QString("  > %1\n").arg(r);
        }
    }

    return report;
}

QString ImageAnalysis::getReport() const
{
    QString report;
    report += QString("Total Images: %1\n").arg(totalImages);
    report += QString("Usable Images: %1 (%2%)\n")
        .arg(usableImages)
        .arg(totalImages > 0 ? (usableImages * 100.0 / totalImages) : 0.0, 0, 'f', 1);

    if (blurryImages > 0) {
        report += QString("Blurry: %1\n").arg(blurryImages);
    }
    if (overexposedImages > 0) {
        report += QString("Overexposed: %1\n").arg(overexposedImages);
    }
    if (underexposedImages > 0) {
        report += QString("Underexposed: %1\n").arg(underexposedImages);
    }

    report += QString("Average Sharpness: %1%\n").arg(averageSharpness, 0, 'f', 1);
    report += QString("Average Exposure: %1%\n").arg(averageExposure, 0, 'f', 1);

    return report;
}

QualityEstimator::QualityEstimator()
{
}

QualityEstimate QualityEstimator::estimateQuality(
    const Models::FlightPlan& plan,
    const Models::MissionParameters& params)
{
    QualityEstimate estimate;

    // Calculate quality factors
    Core::MissionStats stats = Core::MissionStatistics::analyze(plan, params);

    estimate.factors.gsdScore = calculateGSDScore(stats.groundSamplingDistance);
    estimate.factors.overlapScore = calculateOverlapScore(
        params.frontOverlap(), params.sideOverlap());
    estimate.factors.coverageScore = calculateCoverageScore(plan);
    estimate.factors.altitudeScore = calculateAltitudeScore(plan);
    estimate.factors.cameraScore = calculateCameraScore(params.cameraModel());
    estimate.factors.imageCountScore = calculateImageCountScore(
        stats.estimatedPhotoCount, stats.surveyArea);
    estimate.factors.geometryScore = 85.0; // Default - would analyze flight pattern
    estimate.factors.lightingScore = 80.0; // Default - would use weather data

    // Calculate overall score
    estimate.overallScore = estimate.factors.getOverallScore();
    estimate.level = getQualityLevel(estimate.overallScore);

    // Predictions
    double avgOverlap = (params.frontOverlap() + params.sideOverlap()) / 2.0;
    estimate.expectedAccuracy = predictAccuracy(stats.groundSamplingDistance, avgOverlap);
    estimate.expectedDensity = predictDensity(
        stats.groundSamplingDistance,
        stats.estimatedPhotoCount,
        stats.surveyArea);
    estimate.expectedCompleteness = predictCompleteness(
        estimate.factors.overlapScore,
        estimate.factors.coverageScore);
    estimate.meshQuality = estimate.overallScore; // Simplified

    // Generate recommendations
    estimate.strengths = identifyStrengths(estimate.factors);
    estimate.weaknesses = identifyWeaknesses(estimate.factors);
    estimate.recommendations = generateRecommendations(estimate);

    return estimate;
}

double QualityEstimator::calculateGSDScore(double gsd, double targetGSD)
{
    // Optimal GSD is around target (default 2cm for high-quality mapping)
    // Score decreases as GSD deviates from target

    if (gsd <= 0.0) return 0.0;

    double ratio = gsd / targetGSD;

    if (ratio <= 0.5) {
        // Too fine - inefficient but high quality
        return 95.0;
    } else if (ratio <= 1.0) {
        // Excellent - at or better than target
        return 100.0;
    } else if (ratio <= 2.0) {
        // Good - within acceptable range
        return normalizeScore(ratio, 1.0, 2.0, true) * 40.0 + 60.0;
    } else if (ratio <= 4.0) {
        // Acceptable - coarser than ideal
        return normalizeScore(ratio, 2.0, 4.0, true) * 30.0 + 30.0;
    } else {
        // Poor - very coarse
        return std::max(0.0, 30.0 - (ratio - 4.0) * 5.0);
    }
}

double QualityEstimator::calculateOverlapScore(
    double frontOverlap,
    double sideOverlap)
{
    // Ideal overlap: 75-85% front, 65-75% side
    // Minimum acceptable: 60% front, 40% side

    double frontScore = 0.0;
    double sideScore = 0.0;

    // Front overlap score
    if (frontOverlap < 60.0) {
        frontScore = (frontOverlap / 60.0) * 40.0;
    } else if (frontOverlap <= 75.0) {
        frontScore = normalizeScore(frontOverlap, 60.0, 75.0) * 20.0 + 40.0;
    } else if (frontOverlap <= 85.0) {
        frontScore = 100.0;
    } else if (frontOverlap <= 95.0) {
        frontScore = normalizeScore(frontOverlap, 85.0, 95.0, true) * 20.0 + 80.0;
    } else {
        frontScore = 60.0; // Too much overlap - inefficient
    }

    // Side overlap score
    if (sideOverlap < 40.0) {
        sideScore = (sideOverlap / 40.0) * 40.0;
    } else if (sideOverlap <= 65.0) {
        sideScore = normalizeScore(sideOverlap, 40.0, 65.0) * 20.0 + 40.0;
    } else if (sideOverlap <= 75.0) {
        sideScore = 100.0;
    } else if (sideOverlap <= 90.0) {
        sideScore = normalizeScore(sideOverlap, 75.0, 90.0, true) * 20.0 + 80.0;
    } else {
        sideScore = 60.0;
    }

    // Weight front overlap more heavily (60% front, 40% side)
    return frontScore * 0.6 + sideScore * 0.4;
}

double QualityEstimator::calculateCoverageScore(const Models::FlightPlan& plan)
{
    // Check coverage uniformity
    // Simplified - would analyze waypoint spacing consistency
    const auto& waypoints = plan.waypoints();

    if (waypoints.count() < 3) {
        return 50.0;
    }

    // Calculate spacing variance
    QList<double> spacings;
    for (int i = 1; i < waypoints.count(); ++i) {
        // Calculate spacing (simplified)
        spacings.append(1.0); // Placeholder
    }

    // Low variance = high score
    return 85.0; // Default good score
}

double QualityEstimator::calculateAltitudeScore(const Models::FlightPlan& plan)
{
    const auto& waypoints = plan.waypoints();

    if (waypoints.count() < 2) {
        return 100.0;
    }

    // Calculate altitude variance
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

    // Lower variance = higher score
    if (stddev < 1.0) return 100.0;
    if (stddev < 5.0) return 95.0;
    if (stddev < 10.0) return 85.0;
    if (stddev < 20.0) return 70.0;
    if (stddev < 30.0) return 50.0;
    return 30.0;
}

double QualityEstimator::calculateCameraScore(
    Models::MissionParameters::CameraModel cameraModel)
{
    switch (cameraModel) {
    case Models::MissionParameters::CameraModel::DJI_Mavic3:
        return 95.0; // 20MP, 4/3" sensor - excellent
    case Models::MissionParameters::CameraModel::DJI_Air3:
        return 85.0; // 12MP, 1" sensor - very good
    case Models::MissionParameters::CameraModel::DJI_Mini3Pro:
        return 75.0; // 12MP, 1/1.3" sensor - good
    case Models::MissionParameters::CameraModel::DJI_Mini3:
        return 70.0; // 12MP, 1/1.3" sensor - good
    default:
        return 60.0; // Unknown camera
    }
}

double QualityEstimator::calculateImageCountScore(
    int imageCount,
    double surveyAreaSqm)
{
    if (surveyAreaSqm <= 0.0) return 50.0;

    // Ideal: ~100-200 images per hectare (10,000 m²)
    double imagesPerHectare = (imageCount / surveyAreaSqm) * 10000.0;

    if (imagesPerHectare < 50) {
        return (imagesPerHectare / 50.0) * 40.0;
    } else if (imagesPerHectare <= 100) {
        return normalizeScore(imagesPerHectare, 50, 100) * 30.0 + 40.0;
    } else if (imagesPerHectare <= 200) {
        return 100.0;
    } else if (imagesPerHectare <= 400) {
        return normalizeScore(imagesPerHectare, 200, 400, true) * 20.0 + 80.0;
    } else {
        return 70.0; // Too many - inefficient
    }
}

double QualityEstimator::predictAccuracy(double gsd, double overlap)
{
    // Accuracy typically 1-3x GSD for good overlap
    // Better overlap = better accuracy

    double baseAccuracy = gsd * 2.0; // cm

    // Adjust for overlap
    if (overlap >= 75.0) {
        baseAccuracy *= 0.8; // 20% improvement
    } else if (overlap >= 60.0) {
        baseAccuracy *= 1.0; // Nominal
    } else {
        baseAccuracy *= 1.5; // 50% worse
    }

    return baseAccuracy;
}

double QualityEstimator::predictDensity(
    double gsd,
    int imageCount,
    double surveyAreaSqm)
{
    if (surveyAreaSqm <= 0.0) return 0.0;

    // Rough estimation: density inversely proportional to GSD²
    // More images = higher density

    double pixelSizeCm = gsd;
    double pixelSizeM = pixelSizeCm / 100.0;

    // Base density from GSD
    double baseDensity = 1.0 / (pixelSizeM * pixelSizeM);

    // Adjust for image count
    double imagesPerSqm = imageCount / surveyAreaSqm;
    double densityFactor = std::min(imagesPerSqm / 0.01, 2.0); // Cap at 2x

    return baseDensity * densityFactor * 0.5; // Realistic reduction
}

double QualityEstimator::predictCompleteness(
    double overlapScore,
    double coverageScore)
{
    // Higher overlap and better coverage = more complete reconstruction

    double baseCompleteness = 70.0; // Baseline

    // Overlap contribution (30%)
    baseCompleteness += (overlapScore / 100.0) * 20.0;

    // Coverage contribution (10%)
    baseCompleteness += (coverageScore / 100.0) * 10.0;

    return std::min(baseCompleteness, 100.0);
}

QStringList QualityEstimator::generateRecommendations(const QualityEstimate& estimate)
{
    QStringList recs;

    if (estimate.factors.gsdScore < 70.0) {
        recs.append("Increase flight altitude or use a better camera for finer GSD");
    }

    if (estimate.factors.overlapScore < 70.0) {
        recs.append("Increase image overlap (target 75-85% front, 65-75% side)");
    }

    if (estimate.factors.altitudeScore < 70.0) {
        recs.append("Reduce altitude variation for more consistent image quality");
    }

    if (estimate.factors.imageCountScore < 70.0) {
        recs.append("Add more flight lines or reduce line spacing for better coverage");
    }

    if (estimate.factors.cameraScore < 80.0) {
        recs.append("Consider using a higher-quality camera for better results");
    }

    if (estimate.expectedAccuracy > 5.0) {
        recs.append("For better accuracy, reduce GSD by flying lower or using zoom");
    }

    if (estimate.expectedCompleteness < 90.0) {
        recs.append("Increase overlap and ensure complete area coverage");
    }

    if (recs.isEmpty()) {
        recs.append("Mission parameters look excellent - no changes needed");
    }

    return recs;
}

QualityLevel QualityEstimator::getQualityLevel(double score)
{
    if (score >= 90.0) return QualityLevel::Excellent;
    if (score >= 75.0) return QualityLevel::Good;
    if (score >= 60.0) return QualityLevel::Acceptable;
    if (score >= 40.0) return QualityLevel::Poor;
    return QualityLevel::Inadequate;
}

QString QualityEstimator::getQualityLevelName(QualityLevel level)
{
    switch (level) {
    case QualityLevel::Excellent:   return "Excellent";
    case QualityLevel::Good:        return "Good";
    case QualityLevel::Acceptable:  return "Acceptable";
    case QualityLevel::Poor:        return "Poor";
    case QualityLevel::Inadequate:  return "Inadequate";
    }
    return "Unknown";
}

ReconstructionComplexity QualityEstimator::assessComplexity(
    const Models::FlightPlan& plan,
    const Models::MissionParameters& params)
{
    Q_UNUSED(params);

    const auto& waypoints = plan.waypoints();

    if (waypoints.isEmpty()) {
        return ReconstructionComplexity::Simple;
    }

    // Assess based on altitude variation
    double altVariance = 0.0;
    if (waypoints.count() > 1) {
        double sum = 0.0;
        for (const auto& wp : waypoints) {
            sum += wp.coordinate().altitude();
        }
        double mean = sum / waypoints.count();

        for (const auto& wp : waypoints) {
            double diff = wp.coordinate().altitude() - mean;
            altVariance += diff * diff;
        }
        altVariance = std::sqrt(altVariance / waypoints.count());
    }

    if (altVariance < 5.0) return ReconstructionComplexity::Simple;
    if (altVariance < 15.0) return ReconstructionComplexity::Moderate;
    if (altVariance < 30.0) return ReconstructionComplexity::Complex;
    return ReconstructionComplexity::VeryComplex;
}

double QualityEstimator::calculateOptimalGSD(double targetAccuracyCm)
{
    // GSD should be 0.5-1x target accuracy
    return targetAccuracyCm * 0.7;
}

double QualityEstimator::calculateOptimalOverlap(double targetCompleteness)
{
    // Higher completeness requires higher overlap
    if (targetCompleteness >= 95.0) return 85.0;
    if (targetCompleteness >= 90.0) return 80.0;
    if (targetCompleteness >= 85.0) return 75.0;
    if (targetCompleteness >= 80.0) return 70.0;
    return 65.0;
}

double QualityEstimator::normalizeScore(
    double value,
    double min,
    double max,
    bool inverse)
{
    if (max <= min) return 0.0;

    double normalized = (value - min) / (max - min);
    normalized = std::max(0.0, std::min(1.0, normalized));

    return inverse ? (1.0 - normalized) : normalized;
}

QStringList QualityEstimator::identifyStrengths(const QualityFactors& factors)
{
    QStringList strengths;

    if (factors.gsdScore >= 85.0) strengths.append("Excellent ground resolution");
    if (factors.overlapScore >= 85.0) strengths.append("Optimal image overlap");
    if (factors.coverageScore >= 85.0) strengths.append("Uniform coverage");
    if (factors.altitudeScore >= 85.0) strengths.append("Consistent flight altitude");
    if (factors.cameraScore >= 85.0) strengths.append("High-quality camera");
    if (factors.imageCountScore >= 85.0) strengths.append("Sufficient image count");

    return strengths;
}

QStringList QualityEstimator::identifyWeaknesses(const QualityFactors& factors)
{
    QStringList weaknesses;

    if (factors.gsdScore < 60.0) weaknesses.append("Poor ground resolution");
    if (factors.overlapScore < 60.0) weaknesses.append("Insufficient overlap");
    if (factors.coverageScore < 60.0) weaknesses.append("Uneven coverage");
    if (factors.altitudeScore < 60.0) weaknesses.append("Inconsistent altitude");
    if (factors.imageCountScore < 60.0) weaknesses.append("Too few images");

    return weaknesses;
}

} // namespace Photogrammetry
} // namespace DroneMapper
