#include "ImageManager.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QtMath>
#include <algorithm>

namespace DroneMapper {
namespace Core {

// ImageMetadata implementation

ImageMetadata::ImageMetadata()
    : fileSize(0)
    , focalLength(0.0)
    , aperture(0.0)
    , exposureTime(0.0)
    , hasGPS(false)
    , gimbalPitch(0.0)
    , gimbalYaw(0.0)
    , sharpness(0.0)
    , blurScore(0.0)
    , isBlurry(false)
    , brightness(0.0)
{
}

// QualityAssessment implementation

QualityAssessment::QualityAssessment()
    : isAcceptable(true)
    , overallScore(0.0)
    , sharpnessScore(0.0)
    , exposureScore(0.0)
    , noiseScore(0.0)
{
}

// ImageCollectionStats implementation

ImageCollectionStats::ImageCollectionStats()
    : totalImages(0)
    , geotaggedImages(0)
    , acceptableQuality(0)
    , poorQuality(0)
    , totalSize(0)
    , avgSharpness(0.0)
    , avgBrightness(0.0)
{
}

// ImageManager implementation

ImageManager::ImageManager(QObject *parent)
    : QObject(parent)
{
}

ImageManager::~ImageManager()
{
}

int ImageManager::scanDirectory(const QString& directoryPath, bool recursive)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        m_lastError = "Directory does not exist: " + directoryPath;
        return 0;
    }

    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
                << "*.png" << "*.PNG"
                << "*.tif" << "*.tiff" << "*.TIF" << "*.TIFF"
                << "*.dng" << "*.DNG";

    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
    QDirIterator::IteratorFlags iteratorFlags = QDirIterator::NoIteratorFlags;

    if (recursive) {
        iteratorFlags = QDirIterator::Subdirectories;
    }

    QDirIterator it(directoryPath, nameFilters, filters, iteratorFlags);

    QStringList imagePaths;
    while (it.hasNext()) {
        imagePaths.append(it.next());
    }

    int count = 0;
    int total = imagePaths.size();

    for (int i = 0; i < imagePaths.size(); ++i) {
        if (addImage(imagePaths[i])) {
            count++;
        }
        emit scanProgress(i + 1, total);
    }

    return count;
}

bool ImageManager::addImage(const QString& filePath)
{
    if (!isSupportedImageFormat(filePath)) {
        return false;
    }

    // Check if already added
    if (m_imageIndex.contains(filePath)) {
        return false;
    }

    ImageMetadata metadata = extractMetadata(filePath);
    if (metadata.fileName.isEmpty()) {
        return false;
    }

    m_imageIndex[filePath] = m_images.size();
    m_images.append(metadata);

    emit imageAdded(metadata);
    return true;
}

void ImageManager::removeImage(const QString& filePath)
{
    if (!m_imageIndex.contains(filePath)) {
        return;
    }

    int index = m_imageIndex[filePath];
    m_images.removeAt(index);
    m_imageIndex.remove(filePath);

    // Rebuild index
    for (int i = index; i < m_images.size(); ++i) {
        m_imageIndex[m_images[i].filePath] = i;
    }

    emit imageRemoved(filePath);
}

void ImageManager::clear()
{
    m_images.clear();
    m_imageIndex.clear();
}

ImageMetadata ImageManager::imageByPath(const QString& filePath) const
{
    if (m_imageIndex.contains(filePath)) {
        return m_images[m_imageIndex[filePath]];
    }
    return ImageMetadata();
}

QVector<ImageMetadata> ImageManager::geotaggedImages() const
{
    QVector<ImageMetadata> result;
    for (const auto& img : m_images) {
        if (img.hasGPS) {
            result.append(img);
        }
    }
    return result;
}

QVector<ImageMetadata> ImageManager::qualityImages() const
{
    QVector<ImageMetadata> result;
    for (const auto& img : m_images) {
        if (!img.isBlurry && img.sharpness > 50.0) {
            result.append(img);
        }
    }
    return result;
}

ImageCollectionStats ImageManager::statistics() const
{
    ImageCollectionStats stats;

    if (m_images.isEmpty()) {
        return stats;
    }

    stats.totalImages = m_images.size();

    double totalSharpness = 0.0;
    double totalBrightness = 0.0;

    for (const auto& img : m_images) {
        stats.totalSize += img.fileSize;

        if (img.hasGPS) {
            stats.geotaggedImages++;
        }

        if (!img.isBlurry && img.sharpness > 50.0) {
            stats.acceptableQuality++;
        } else {
            stats.poorQuality++;
        }

        totalSharpness += img.sharpness;
        totalBrightness += img.brightness;

        if (stats.earliestCapture.isNull() || img.captureTime < stats.earliestCapture) {
            stats.earliestCapture = img.captureTime;
        }
        if (stats.latestCapture.isNull() || img.captureTime > stats.latestCapture) {
            stats.latestCapture = img.captureTime;
        }
    }

    stats.avgSharpness = totalSharpness / stats.totalImages;
    stats.avgBrightness = totalBrightness / stats.totalImages;

    return stats;
}

QualityAssessment ImageManager::assessQuality(const QString& filePath)
{
    QImage image(filePath);
    if (image.isNull()) {
        m_lastError = "Failed to load image: " + filePath;
        QualityAssessment assessment;
        assessment.isAcceptable = false;
        assessment.issues << "Failed to load image";
        return assessment;
    }

    ImageMetadata metadata = imageByPath(filePath);
    return performQualityCheck(metadata, image);
}

int ImageManager::batchGeotagFromLog(const QString& logFilePath)
{
    // Placeholder for flight log parsing
    // This would parse DJI flight logs, Litchi CSV, or similar formats
    // and match images by timestamp to GPS coordinates

    Q_UNUSED(logFilePath);
    m_lastError = "Batch geotagging not yet implemented";
    return 0;
}

QImage ImageManager::generateThumbnail(const QString& filePath, int maxSize)
{
    QImage image(filePath);
    if (image.isNull()) {
        return QImage();
    }

    return image.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool ImageManager::exportToKML(const QString& outputPath)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Failed to open file for writing: " + outputPath;
        return false;
    }

    QTextStream out(&file);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
    out << "<Document>\n";
    out << "  <name>Drone Images</name>\n";

    for (const auto& img : m_images) {
        if (!img.hasGPS) {
            continue;
        }

        out << "  <Placemark>\n";
        out << "    <name>" << img.fileName << "</name>\n";
        out << "    <description>\n";
        out << "      <![CDATA[\n";
        out << "      <p>Captured: " << img.captureTime.toString(Qt::ISODate) << "</p>\n";
        out << "      <p>Camera: " << img.cameraMake << " " << img.cameraModel << "</p>\n";
        out << "      <p>Dimensions: " << img.dimensions.width() << "x" << img.dimensions.height() << "</p>\n";
        out << "      <p>Sharpness: " << QString::number(img.sharpness, 'f', 1) << "</p>\n";
        out << "      ]]>\n";
        out << "    </description>\n";
        out << "    <Point>\n";
        out << "      <coordinates>" << img.coordinate.longitude() << ","
            << img.coordinate.latitude() << "," << img.coordinate.altitude() << "</coordinates>\n";
        out << "    </Point>\n";
        out << "  </Placemark>\n";
    }

    out << "</Document>\n";
    out << "</kml>\n";

    file.close();
    return true;
}

ImageMetadata ImageManager::extractMetadata(const QString& filePath)
{
    ImageMetadata metadata;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return metadata;
    }

    metadata.filePath = filePath;
    metadata.fileName = fileInfo.fileName();
    metadata.fileSize = fileInfo.size();
    metadata.captureTime = fileInfo.birthTime();

    // Load image for dimension and quality analysis
    QImageReader reader(filePath);
    metadata.dimensions = reader.size();

    QImage image = reader.read();
    if (!image.isNull()) {
        // Quality metrics
        metadata.sharpness = calculateSharpness(image);
        metadata.blurScore = calculateBlur(image);
        metadata.brightness = calculateBrightness(image);
        metadata.isBlurry = (metadata.blurScore > 50.0 || metadata.sharpness < 30.0);
    }

    // Extract EXIF data
    extractEXIF(filePath, metadata);

    // Parse GPS data
    metadata.coordinate = parseGPSData(filePath);
    metadata.hasGPS = (metadata.coordinate.latitude() != 0.0 || metadata.coordinate.longitude() != 0.0);

    return metadata;
}

bool ImageManager::extractEXIF(const QString& filePath, ImageMetadata& metadata)
{
    // Simplified EXIF extraction using Qt
    // For production, use libexif or ExifTool

    Q_UNUSED(filePath);

    // Placeholder values
    metadata.cameraMake = "Unknown";
    metadata.cameraModel = "Unknown";
    metadata.focalLength = 0.0;
    metadata.aperture = 0.0;
    metadata.iso = "0";
    metadata.exposureTime = 0.0;

    // In production, parse EXIF data here using external library
    // Example with ExifTool:
    // exiftool -Make -Model -FocalLength -FNumber -ISO -ExposureTime filePath

    return true;
}

Models::GeospatialCoordinate ImageManager::parseGPSData(const QString& filePath)
{
    // Simplified GPS parsing
    // For production, use libexif or ExifTool to extract GPS coordinates

    Q_UNUSED(filePath);

    // Placeholder - return zero coordinates
    // In production, parse GPS EXIF tags:
    // GPSLatitude, GPSLongitude, GPSAltitude

    return Models::GeospatialCoordinate(0.0, 0.0, 0.0);
}

double ImageManager::calculateSharpness(const QImage& image)
{
    // Laplacian variance method for sharpness detection
    if (image.isNull()) {
        return 0.0;
    }

    // Convert to grayscale
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    int width = gray.width();
    int height = gray.height();

    if (width < 3 || height < 3) {
        return 0.0;
    }

    // Sample center region for performance
    int sampleSize = qMin(200, qMin(width, height));
    int offsetX = (width - sampleSize) / 2;
    int offsetY = (height - sampleSize) / 2;

    QVector<double> laplacian;

    for (int y = offsetY + 1; y < offsetY + sampleSize - 1; y++) {
        const uchar* row0 = gray.constScanLine(y - 1);
        const uchar* row1 = gray.constScanLine(y);
        const uchar* row2 = gray.constScanLine(y + 1);

        for (int x = offsetX + 1; x < offsetX + sampleSize - 1; x++) {
            // Laplacian kernel
            double val = -row0[x - 1] - row0[x] - row0[x + 1]
                        - row1[x - 1] + 8 * row1[x] - row1[x + 1]
                        - row2[x - 1] - row2[x] - row2[x + 1];
            laplacian.append(val);
        }
    }

    if (laplacian.isEmpty()) {
        return 0.0;
    }

    // Calculate variance
    double mean = 0.0;
    for (double val : laplacian) {
        mean += val;
    }
    mean /= laplacian.size();

    double variance = 0.0;
    for (double val : laplacian) {
        variance += (val - mean) * (val - mean);
    }
    variance /= laplacian.size();

    // Normalize to 0-100 scale
    double sharpness = qSqrt(variance) / 10.0;
    return qMin(100.0, sharpness);
}

double ImageManager::calculateBlur(const QImage& image)
{
    // Inverse of sharpness
    return 100.0 - calculateSharpness(image);
}

double ImageManager::calculateBrightness(const QImage& image)
{
    if (image.isNull()) {
        return 0.0;
    }

    // Convert to grayscale
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    qint64 totalBrightness = 0;
    int pixelCount = 0;

    // Sample for performance
    int step = qMax(1, qMin(gray.width(), gray.height()) / 100);

    for (int y = 0; y < gray.height(); y += step) {
        const uchar* row = gray.constScanLine(y);
        for (int x = 0; x < gray.width(); x += step) {
            totalBrightness += row[x];
            pixelCount++;
        }
    }

    if (pixelCount == 0) {
        return 0.0;
    }

    return static_cast<double>(totalBrightness) / pixelCount;
}

QualityAssessment ImageManager::performQualityCheck(const ImageMetadata& metadata, const QImage& image)
{
    QualityAssessment assessment;

    // Sharpness check
    assessment.sharpnessScore = metadata.sharpness;
    if (metadata.sharpness < 30.0) {
        assessment.issues << "Image is too blurry (sharpness: " +
            QString::number(metadata.sharpness, 'f', 1) + ")";
        assessment.isAcceptable = false;
    } else if (metadata.sharpness < 50.0) {
        assessment.warnings << "Image sharpness is marginal";
    }

    // Exposure check
    assessment.exposureScore = 100.0 - qAbs(metadata.brightness - 127.5) / 1.275;
    if (metadata.brightness < 30.0) {
        assessment.issues << "Image is too dark (brightness: " +
            QString::number(metadata.brightness, 'f', 0) + ")";
        assessment.isAcceptable = false;
    } else if (metadata.brightness > 225.0) {
        assessment.issues << "Image is too bright (brightness: " +
            QString::number(metadata.brightness, 'f', 0) + ")";
        assessment.isAcceptable = false;
    } else if (metadata.brightness < 60.0 || metadata.brightness > 195.0) {
        assessment.warnings << "Image exposure is marginal";
    }

    // Noise estimation (simplified)
    assessment.noiseScore = 80.0;  // Placeholder

    // Resolution check
    int pixelCount = metadata.dimensions.width() * metadata.dimensions.height();
    if (pixelCount < 2000000) {  // < 2MP
        assessment.warnings << "Low resolution image (< 2MP)";
    }

    // Overall score
    assessment.overallScore = (assessment.sharpnessScore + assessment.exposureScore + assessment.noiseScore) / 3.0;

    return assessment;
}

bool ImageManager::isSupportedImageFormat(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    QStringList supported;
    supported << "jpg" << "jpeg" << "png" << "tif" << "tiff" << "dng";

    return supported.contains(ext);
}

} // namespace Core
} // namespace DroneMapper
