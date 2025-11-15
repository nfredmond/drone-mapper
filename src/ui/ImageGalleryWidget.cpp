#include "ImageGalleryWidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QDateTime>

namespace DroneMapper {
namespace UI {

// ImageThumbnailItem implementation

ImageThumbnailItem::ImageThumbnailItem(const Core::ImageMetadata& metadata, QListWidget *parent)
    : QListWidgetItem(parent)
    , m_metadata(metadata)
{
    setText(metadata.fileName);
    
    // Load and set thumbnail
    QImage thumbnail = QImage(metadata.filePath).scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!thumbnail.isNull()) {
        setIcon(QIcon(QPixmap::fromImage(thumbnail)));
    }
    
    // Set tooltip with metadata
    QString tooltip = QString("File: %1\nSize: %2x%3\nSharpness: %4")
        .arg(metadata.fileName)
        .arg(metadata.dimensions.width())
        .arg(metadata.dimensions.height())
        .arg(metadata.sharpness, 0, 'f', 1);
    setToolTip(tooltip);
}

// ImageGalleryWidget implementation

ImageGalleryWidget::ImageGalleryWidget(QWidget *parent)
    : QWidget(parent)
    , m_imageManager(new Core::ImageManager(this))
    , m_filterQuality(false)
    , m_filterGeotagged(false)
{
    setupUI();
    
    // Connect image manager signals
    connect(m_imageManager, &Core::ImageManager::scanProgress,
            this, &ImageGalleryWidget::onScanProgress);
    connect(m_imageManager, &Core::ImageManager::imageAdded,
            this, &ImageGalleryWidget::onImageAdded);
    
    setWindowTitle("Image Gallery");
    resize(1200, 800);
}

ImageGalleryWidget::~ImageGalleryWidget()
{
}

void ImageGalleryWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    m_toolbarLayout = new QHBoxLayout();
    
    m_loadDirectoryButton = new QPushButton("Load Directory...", this);
    connect(m_loadDirectoryButton, &QPushButton::clicked, this, &ImageGalleryWidget::onLoadDirectoryClicked);
    
    m_clearButton = new QPushButton("Clear", this);
    connect(m_clearButton, &QPushButton::clicked, this, &ImageGalleryWidget::onClearClicked);
    
    m_exportKMLButton = new QPushButton("Export to KML", this);
    connect(m_exportKMLButton, &QPushButton::clicked, this, &ImageGalleryWidget::onExportKMLClicked);
    
    m_filterQualityCheckbox = new QCheckBox("Quality Only", this);
    connect(m_filterQualityCheckbox, &QCheckBox::stateChanged, this, &ImageGalleryWidget::onFilterQualityChanged);
    
    m_filterGeotaggedCheckbox = new QCheckBox("Geotagged Only", this);
    connect(m_filterGeotaggedCheckbox, &QCheckBox::stateChanged, this, &ImageGalleryWidget::onFilterGeotaggedChanged);
    
    m_toolbarLayout->addWidget(m_loadDirectoryButton);
    m_toolbarLayout->addWidget(m_clearButton);
    m_toolbarLayout->addWidget(m_exportKMLButton);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_filterQualityCheckbox);
    m_toolbarLayout->addWidget(m_filterGeotaggedCheckbox);
    
    m_mainLayout->addLayout(m_toolbarLayout);
    
    // Progress bar
    m_scanProgress = new QProgressBar(this);
    m_scanProgress->setVisible(false);
    m_mainLayout->addWidget(m_scanProgress);
    
    // Main content area
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    // Thumbnail list
    m_thumbnailList = new QListWidget(this);
    m_thumbnailList->setViewMode(QListWidget::IconMode);
    m_thumbnailList->setIconSize(QSize(150, 150));
    m_thumbnailList->setResizeMode(QListWidget::Adjust);
    m_thumbnailList->setSpacing(10);
    connect(m_thumbnailList, &QListWidget::itemClicked, this, &ImageGalleryWidget::onImageItemClicked);
    
    contentLayout->addWidget(m_thumbnailList, 2);
    
    // Info panel
    QVBoxLayout *infoPanelLayout = new QVBoxLayout();
    
    // Metadata panel
    m_metadataGroup = new QGroupBox("Image Metadata", this);
    QGridLayout *metadataLayout = new QGridLayout();
    
    m_fileNameLabel = new QLabel("File: --", this);
    m_fileSizeLabel = new QLabel("Size: --", this);
    m_dimensionsLabel = new QLabel("Dimensions: --", this);
    m_captureTimeLabel = new QLabel("Captured: --", this);
    m_cameraLabel = new QLabel("Camera: --", this);
    m_gpsLabel = new QLabel("GPS: --", this);
    m_sharpnessLabel = new QLabel("Sharpness: --", this);
    m_brightnessLabel = new QLabel("Brightness: --", this);
    m_qualityLabel = new QLabel("Quality: --", this);
    
    metadataLayout->addWidget(m_fileNameLabel, 0, 0);
    metadataLayout->addWidget(m_fileSizeLabel, 1, 0);
    metadataLayout->addWidget(m_dimensionsLabel, 2, 0);
    metadataLayout->addWidget(m_captureTimeLabel, 3, 0);
    metadataLayout->addWidget(m_cameraLabel, 4, 0);
    metadataLayout->addWidget(m_gpsLabel, 5, 0);
    metadataLayout->addWidget(m_sharpnessLabel, 6, 0);
    metadataLayout->addWidget(m_brightnessLabel, 7, 0);
    metadataLayout->addWidget(m_qualityLabel, 8, 0);
    
    m_metadataGroup->setLayout(metadataLayout);
    infoPanelLayout->addWidget(m_metadataGroup);
    
    // Statistics panel
    m_statsGroup = new QGroupBox("Collection Statistics", this);
    QGridLayout *statsLayout = new QGridLayout();
    
    m_totalImagesLabel = new QLabel("Total Images: 0", this);
    m_geotaggedLabel = new QLabel("Geotagged: 0", this);
    m_qualityCountLabel = new QLabel("Quality: 0", this);
    m_totalSizeLabel = new QLabel("Total Size: 0 MB", this);
    m_avgSharpnessLabel = new QLabel("Avg Sharpness: --", this);
    m_timeRangeLabel = new QLabel("Time Range: --", this);
    
    statsLayout->addWidget(m_totalImagesLabel, 0, 0);
    statsLayout->addWidget(m_geotaggedLabel, 1, 0);
    statsLayout->addWidget(m_qualityCountLabel, 2, 0);
    statsLayout->addWidget(m_totalSizeLabel, 3, 0);
    statsLayout->addWidget(m_avgSharpnessLabel, 4, 0);
    statsLayout->addWidget(m_timeRangeLabel, 5, 0);
    
    m_statsGroup->setLayout(statsLayout);
    infoPanelLayout->addWidget(m_statsGroup);
    
    infoPanelLayout->addStretch();
    
    contentLayout->addLayout(infoPanelLayout, 1);
    
    m_mainLayout->addLayout(contentLayout);
}

void ImageGalleryWidget::loadDirectory(const QString& directoryPath, bool recursive)
{
    m_scanProgress->setVisible(true);
    m_scanProgress->setValue(0);
    
    int count = m_imageManager->scanDirectory(directoryPath, recursive);
    
    m_scanProgress->setVisible(false);
    
    updateThumbnails();
    updateStatisticsPanel();

    // Note: Status message could be emitted via signal if needed
}

void ImageGalleryWidget::onLoadDirectoryClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Image Directory"),
                                                      QDir::homePath(),
                                                      QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) {
        return;
    }
    
    loadDirectory(dir, true);
}

void ImageGalleryWidget::onClearClicked()
{
    m_imageManager->clear();
    m_thumbnailList->clear();
    updateStatisticsPanel();
}

void ImageGalleryWidget::onFilterQualityChanged(int state)
{
    m_filterQuality = (state == Qt::Checked);
    updateThumbnails();
}

void ImageGalleryWidget::onFilterGeotaggedChanged(int state)
{
    m_filterGeotagged = (state == Qt::Checked);
    updateThumbnails();
}

void ImageGalleryWidget::onImageItemClicked(QListWidgetItem *item)
{
    ImageThumbnailItem *thumbItem = dynamic_cast<ImageThumbnailItem*>(item);
    if (!thumbItem) {
        return;
    }
    
    updateMetadataPanel(thumbItem->metadata());
    emit imageSelected(thumbItem->metadata());
    
    if (thumbItem->metadata().hasGPS) {
        emit showImageLocation(thumbItem->metadata().coordinate);
    }
}

void ImageGalleryWidget::onScanProgress(int current, int total)
{
    m_scanProgress->setMaximum(total);
    m_scanProgress->setValue(current);
}

void ImageGalleryWidget::onImageAdded(const Core::ImageMetadata& metadata)
{
    // Update will be done in batch after scan completes
    Q_UNUSED(metadata);
}

void ImageGalleryWidget::onExportKMLClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export to KML"),
        QDir::homePath() + "/images.kml",
        tr("KML Files (*.kml)"));
    
    if (fileName.isEmpty()) {
        return;
    }
    
    if (m_imageManager->exportToKML(fileName)) {
        QMessageBox::information(this, tr("Export Successful"),
            tr("Images exported to KML successfully!"));
    } else {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Failed to export images:\n%1").arg(m_imageManager->lastError()));
    }
}

void ImageGalleryWidget::onRefreshStatistics()
{
    updateStatisticsPanel();
}

void ImageGalleryWidget::updateThumbnails()
{
    m_thumbnailList->clear();
    
    QVector<Core::ImageMetadata> images = m_imageManager->images();
    
    if (m_filterGeotagged) {
        images = m_imageManager->geotaggedImages();
    }
    
    if (m_filterQuality) {
        images = m_imageManager->qualityImages();
    }
    
    for (const auto& img : images) {
        new ImageThumbnailItem(img, m_thumbnailList);
    }
}

void ImageGalleryWidget::updateMetadataPanel(const Core::ImageMetadata& metadata)
{
    m_fileNameLabel->setText("File: " + metadata.fileName);
    m_fileSizeLabel->setText("Size: " + formatFileSize(metadata.fileSize));
    m_dimensionsLabel->setText(QString("Dimensions: %1x%2")
        .arg(metadata.dimensions.width()).arg(metadata.dimensions.height()));
    m_captureTimeLabel->setText("Captured: " + metadata.captureTime.toString("yyyy-MM-dd hh:mm:ss"));
    m_cameraLabel->setText(QString("Camera: %1 %2").arg(metadata.cameraMake).arg(metadata.cameraModel));
    
    if (metadata.hasGPS) {
        m_gpsLabel->setText(QString("GPS: %1, %2 (%3m)")
            .arg(metadata.coordinate.latitude(), 0, 'f', 6)
            .arg(metadata.coordinate.longitude(), 0, 'f', 6)
            .arg(metadata.coordinate.altitude(), 0, 'f', 1));
    } else {
        m_gpsLabel->setText("GPS: Not available");
    }
    
    m_sharpnessLabel->setText(QString("Sharpness: %1").arg(metadata.sharpness, 0, 'f', 1));
    m_brightnessLabel->setText(QString("Brightness: %1").arg(metadata.brightness, 0, 'f', 1));
    
    QString quality = metadata.isBlurry ? "Poor (Blurry)" : "Good";
    m_qualityLabel->setText("Quality: " + quality);
}

void ImageGalleryWidget::updateStatisticsPanel()
{
    auto stats = m_imageManager->statistics();
    
    m_totalImagesLabel->setText(QString("Total Images: %1").arg(stats.totalImages));
    m_geotaggedLabel->setText(QString("Geotagged: %1").arg(stats.geotaggedImages));
    m_qualityCountLabel->setText(QString("Quality: %1 / Poor: %2")
        .arg(stats.acceptableQuality).arg(stats.poorQuality));
    m_totalSizeLabel->setText("Total Size: " + formatFileSize(stats.totalSize));
    m_avgSharpnessLabel->setText(QString("Avg Sharpness: %1").arg(stats.avgSharpness, 0, 'f', 1));
    
    if (stats.earliestCapture.isValid() && stats.latestCapture.isValid()) {
        m_timeRangeLabel->setText(QString("Time Range: %1 to %2")
            .arg(stats.earliestCapture.toString("yyyy-MM-dd"))
            .arg(stats.latestCapture.toString("yyyy-MM-dd")));
    } else {
        m_timeRangeLabel->setText("Time Range: --");
    }
}

QString ImageGalleryWidget::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}

} // namespace UI
} // namespace DroneMapper
