#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include "models/GeospatialCoordinate.h"

namespace DroneMapper {
namespace Core {

/**
 * @brief Image metadata information
 */
struct ImageMetadata {
    QString filePath;
    QString fileName;
    qint64 fileSize;              // bytes
    QDateTime captureTime;
    QSize dimensions;             // width x height

    // EXIF data
    QString cameraMake;
    QString cameraModel;
    double focalLength;           // mm
    double aperture;              // f-number
    QString iso;
    double exposureTime;          // seconds

    // Geospatial data
    bool hasGPS;
    Models::GeospatialCoordinate coordinate;
    double gimbalPitch;           // degrees
    double gimbalYaw;             // degrees

    // Quality metrics
    double sharpness;             // 0-100
    double blurScore;             // 0-100 (lower is better)
    bool isBlurry;
    double brightness;            // 0-255

    ImageMetadata();
};

/**
 * @brief Image quality assessment results
 */
struct QualityAssessment {
    bool isAcceptable;
    double overallScore;          // 0-100
    double sharpnessScore;
    double exposureScore;
    double noiseScore;
    QStringList issues;
    QStringList warnings;

    QualityAssessment();
};

/**
 * @brief Image collection statistics
 */
struct ImageCollectionStats {
    int totalImages;
    int geotaggedImages;
    int acceptableQuality;
    int poorQuality;
    qint64 totalSize;              // bytes
    double avgSharpness;
    double avgBrightness;
    QDateTime earliestCapture;
    QDateTime latestCapture;

    ImageCollectionStats();
};

/**
 * @brief Image Management System
 *
 * Comprehensive image organization and analysis system:
 * - EXIF metadata extraction
 * - GPS coordinate parsing
 * - Image quality assessment
 * - Thumbnail generation
 * - Batch geotagging
 * - Quality filtering
 * - Collection statistics
 *
 * Usage:
 *   ImageManager manager;
 *   manager.scanDirectory("/path/to/images");
 *
 *   auto images = manager.images();
 *   for (const auto& img : images) {
 *       qDebug() << img.fileName << img.coordinate;
 *   }
 */
class ImageManager : public QObject {
    Q_OBJECT

public:
    explicit ImageManager(QObject *parent = nullptr);
    ~ImageManager();

    /**
     * @brief Scan directory for images
     * @param directoryPath Directory path
     * @param recursive Scan subdirectories
     * @return Number of images found
     */
    int scanDirectory(const QString& directoryPath, bool recursive = false);

    /**
     * @brief Add single image
     * @param filePath Image file path
     * @return True if added successfully
     */
    bool addImage(const QString& filePath);

    /**
     * @brief Remove image from collection
     * @param filePath Image file path
     */
    void removeImage(const QString& filePath);

    /**
     * @brief Clear all images
     */
    void clear();

    /**
     * @brief Get all images
     * @return List of image metadata
     */
    QVector<ImageMetadata> images() const { return m_images; }

    /**
     * @brief Get image by path
     * @param filePath Image file path
     * @return Image metadata
     */
    ImageMetadata imageByPath(const QString& filePath) const;

    /**
     * @brief Get geotagged images only
     * @return List of geotagged images
     */
    QVector<ImageMetadata> geotaggedImages() const;

    /**
     * @brief Get images with acceptable quality
     * @return List of quality images
     */
    QVector<ImageMetadata> qualityImages() const;

    /**
     * @brief Get collection statistics
     * @return Statistics
     */
    ImageCollectionStats statistics() const;

    /**
     * @brief Assess image quality
     * @param filePath Image file path
     * @return Quality assessment
     */
    QualityAssessment assessQuality(const QString& filePath);

    /**
     * @brief Batch geotag images from flight log
     * @param logFilePath Flight log file
     * @return Number of images tagged
     */
    int batchGeotagFromLog(const QString& logFilePath);

    /**
     * @brief Generate thumbnail
     * @param filePath Image file path
     * @param maxSize Maximum dimension
     * @return Thumbnail image
     */
    QImage generateThumbnail(const QString& filePath, int maxSize = 200);

    /**
     * @brief Export geotagged images to KML
     * @param outputPath Output KML file
     * @return True if successful
     */
    bool exportToKML(const QString& outputPath);

    /**
     * @brief Get last error message
     * @return Error message
     */
    QString lastError() const { return m_lastError; }

signals:
    /**
     * @brief Emitted during directory scan progress
     * @param current Current image number
     * @param total Total images
     */
    void scanProgress(int current, int total);

    /**
     * @brief Emitted when image is added
     * @param metadata Image metadata
     */
    void imageAdded(const ImageMetadata& metadata);

    /**
     * @brief Emitted when image is removed
     * @param filePath Image file path
     */
    void imageRemoved(const QString& filePath);

private:
    QVector<ImageMetadata> m_images;
    QMap<QString, int> m_imageIndex;  // filePath -> index in m_images
    QString m_lastError;

    // Helper methods
    ImageMetadata extractMetadata(const QString& filePath);
    bool extractEXIF(const QString& filePath, ImageMetadata& metadata);
    Models::GeospatialCoordinate parseGPSData(const QString& filePath);
    double calculateSharpness(const QImage& image);
    double calculateBlur(const QImage& image);
    double calculateBrightness(const QImage& image);
    QualityAssessment performQualityCheck(const ImageMetadata& metadata, const QImage& image);
    bool isSupportedImageFormat(const QString& filePath);
};

} // namespace Core
} // namespace DroneMapper

#endif // IMAGEMANAGER_H
