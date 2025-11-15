#ifndef IMAGEGALLERYWIDGET_H
#define IMAGEGALLERYWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include "core/ImageManager.h"

namespace DroneMapper {
namespace UI {

/**
 * @brief Custom list widget item for image thumbnails
 */
class ImageThumbnailItem : public QListWidgetItem {
public:
    ImageThumbnailItem(const Core::ImageMetadata& metadata, QListWidget *parent = nullptr);

    Core::ImageMetadata metadata() const { return m_metadata; }

private:
    Core::ImageMetadata m_metadata;
};

/**
 * @brief Image Gallery Widget
 *
 * Comprehensive image management interface:
 * - Thumbnail grid view
 * - Metadata display panel
 * - Quality filtering
 * - Batch operations
 * - Export capabilities
 * - GPS visualization
 *
 * Usage:
 *   ImageGalleryWidget *gallery = new ImageGalleryWidget(this);
 *   gallery->loadDirectory("/path/to/images");
 *   gallery->show();
 */
class ImageGalleryWidget : public QWidget {
    Q_OBJECT

public:
    explicit ImageGalleryWidget(QWidget *parent = nullptr);
    ~ImageGalleryWidget();

    /**
     * @brief Load images from directory
     * @param directoryPath Directory path
     * @param recursive Scan subdirectories
     */
    void loadDirectory(const QString& directoryPath, bool recursive = false);

    /**
     * @brief Get image manager
     * @return Image manager instance
     */
    Core::ImageManager* imageManager() { return m_imageManager; }

signals:
    /**
     * @brief Emitted when image is selected
     * @param metadata Image metadata
     */
    void imageSelected(const Core::ImageMetadata& metadata);

    /**
     * @brief Emitted when geotagged image is clicked
     * @param coordinate GPS coordinate
     */
    void showImageLocation(const Models::GeospatialCoordinate& coordinate);

private slots:
    void onLoadDirectoryClicked();
    void onClearClicked();
    void onFilterQualityChanged(int state);
    void onFilterGeotaggedChanged(int state);
    void onImageItemClicked(QListWidgetItem *item);
    void onScanProgress(int current, int total);
    void onImageAdded(const Core::ImageMetadata& metadata);
    void onExportKMLClicked();
    void onRefreshStatistics();

private:
    void setupUI();
    void updateThumbnails();
    void updateMetadataPanel(const Core::ImageMetadata& metadata);
    void updateStatisticsPanel();
    QString formatFileSize(qint64 bytes);

    // Core component
    Core::ImageManager *m_imageManager;

    // UI Components
    QVBoxLayout *m_mainLayout;

    // Toolbar
    QHBoxLayout *m_toolbarLayout;
    QPushButton *m_loadDirectoryButton;
    QPushButton *m_clearButton;
    QPushButton *m_exportKMLButton;
    QCheckBox *m_filterQualityCheckbox;
    QCheckBox *m_filterGeotaggedCheckbox;

    // Thumbnail view
    QListWidget *m_thumbnailList;
    QProgressBar *m_scanProgress;

    // Info panels
    QGroupBox *m_metadataGroup;
    QLabel *m_fileNameLabel;
    QLabel *m_fileSizeLabel;
    QLabel *m_dimensionsLabel;
    QLabel *m_captureTimeLabel;
    QLabel *m_cameraLabel;
    QLabel *m_gpsLabel;
    QLabel *m_sharpnessLabel;
    QLabel *m_brightnessLabel;
    QLabel *m_qualityLabel;

    QGroupBox *m_statsGroup;
    QLabel *m_totalImagesLabel;
    QLabel *m_geotaggedLabel;
    QLabel *m_qualityCountLabel;
    QLabel *m_totalSizeLabel;
    QLabel *m_avgSharpnessLabel;
    QLabel *m_timeRangeLabel;

    // State
    bool m_filterQuality;
    bool m_filterGeotagged;
};

} // namespace UI
} // namespace DroneMapper

#endif // IMAGEGALLERYWIDGET_H
