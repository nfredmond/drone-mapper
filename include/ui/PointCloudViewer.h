#ifndef POINTCLOUDVIEWER_H
#define POINTCLOUDVIEWER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <QString>
#include <QVector>

namespace DroneMapper {
namespace UI {

/**
 * @brief Point in a point cloud
 */
struct Point {
    QVector3D position;
    QColor color;
    QVector3D normal;
    float intensity;        // 0.0-1.0
    int classification;     // LAS classification code

    Point()
        : position(0, 0, 0)
        , color(255, 255, 255)
        , normal(0, 0, 1)
        , intensity(1.0f)
        , classification(0)
    {}
};

/**
 * @brief Point cloud data structure
 */
struct PointCloud {
    QVector<Point> points;
    QString fileName;
    QVector3D minBounds;
    QVector3D maxBounds;
    QVector3D centroid;

    bool hasColors;
    bool hasNormals;
    bool hasIntensity;
    bool hasClassification;

    /**
     * @brief Get number of points
     * @return Point count
     */
    int size() const { return points.size(); }

    /**
     * @brief Check if empty
     * @return True if empty
     */
    bool isEmpty() const { return points.isEmpty(); }

    /**
     * @brief Clear all points
     */
    void clear();

    /**
     * @brief Calculate bounds
     */
    void calculateBounds();

    /**
     * @brief Calculate centroid
     */
    void calculateCentroid();

    /**
     * @brief Load from PLY file
     * @param filePath Path to PLY file
     * @return True if loaded successfully
     */
    bool loadFromPLY(const QString& filePath);

    /**
     * @brief Load from LAS file
     * @param filePath Path to LAS file
     * @return True if loaded successfully
     */
    bool loadFromLAS(const QString& filePath);

    /**
     * @brief Load from XYZ file
     * @param filePath Path to XYZ file
     * @return True if loaded successfully
     */
    bool loadFromXYZ(const QString& filePath);

    /**
     * @brief Save to PLY file
     * @param filePath Output path
     * @return True if saved successfully
     */
    bool saveToPLY(const QString& filePath) const;

    /**
     * @brief Generate test point cloud
     * @param numPoints Number of points to generate
     */
    void generateTestCloud(int numPoints = 10000);
};

/**
 * @brief Point cloud rendering settings
 */
struct PointCloudSettings {
    bool showPoints;
    float pointSize;
    bool usePointColor;
    bool useColorMap;
    bool showNormals;
    float normalLength;

    bool enableLighting;
    bool enableDepthTest;

    QColor defaultColor;
    float opacity;

    // Filtering
    bool enableFiltering;
    float minIntensity;
    float maxIntensity;
    QVector<int> visibleClassifications;

    // LOD (Level of Detail)
    bool enableLOD;
    float lodDistance1;      // Full detail
    float lodDistance2;      // Medium detail
    float lodDistance3;      // Low detail

    PointCloudSettings();
};

/**
 * @brief Color mapping schemes for point clouds
 */
class PointCloudColorMap {
public:
    enum Scheme {
        Height,         // Color by Z coordinate
        Intensity,      // Color by intensity value
        Classification, // Color by LAS classification
        Normal,         // Color by normal direction
        RGB,            // Use point colors
        Uniform         // Single color
    };

    /**
     * @brief Get color for height
     * @param z Z coordinate
     * @param minZ Minimum Z
     * @param maxZ Maximum Z
     * @return Color
     */
    static QColor getColorForHeight(float z, float minZ, float maxZ);

    /**
     * @brief Get color for intensity
     * @param intensity Intensity value (0-1)
     * @return Color
     */
    static QColor getColorForIntensity(float intensity);

    /**
     * @brief Get color for LAS classification
     * @param classification LAS classification code
     * @return Color
     */
    static QColor getColorForClassification(int classification);

    /**
     * @brief Get color for normal
     * @param normal Normal vector
     * @return Color
     */
    static QColor getColorForNormal(const QVector3D& normal);
};

/**
 * @brief Octree node for spatial indexing
 */
struct OctreeNode {
    QVector3D center;
    float halfSize;
    QVector<int> pointIndices;
    QVector<OctreeNode*> children;  // 8 children

    bool isLeaf() const { return children.isEmpty(); }
};

/**
 * @brief Octree for efficient point cloud rendering
 */
class Octree {
public:
    Octree();
    ~Octree();

    /**
     * @brief Build octree from point cloud
     * @param cloud Point cloud
     * @param maxPointsPerNode Maximum points per leaf node
     */
    void build(const PointCloud& cloud, int maxPointsPerNode = 100);

    /**
     * @brief Clear octree
     */
    void clear();

    /**
     * @brief Query points within frustum
     * @param frustumPlanes Frustum planes
     * @return Visible point indices
     */
    QVector<int> queryFrustum(const QVector<QVector4D>& frustumPlanes);

    /**
     * @brief Get LOD level for node
     * @param node Node
     * @param cameraPos Camera position
     * @return LOD level (0 = full, 1 = medium, 2 = low)
     */
    int getLODLevel(const OctreeNode* node, const QVector3D& cameraPos);

private:
    OctreeNode* m_root;
    const PointCloud* m_cloud;

    void buildNode(OctreeNode* node, const QVector<int>& indices, int maxPointsPerNode);
    void queryNode(OctreeNode* node, const QVector<QVector4D>& planes, QVector<int>& result);
    void deleteNode(OctreeNode* node);
};

/**
 * @brief Camera for point cloud viewing
 */
class PointCloudCamera {
public:
    PointCloudCamera();

    void setTarget(const QVector3D& target);
    void orbit(float deltaAzimuth, float deltaElevation);
    void zoom(float delta);
    void pan(float deltaX, float deltaY);
    void reset();

    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix(float aspect) const;

    QVector3D position() const { return m_position; }
    QVector3D target() const { return m_target; }

private:
    QVector3D m_position;
    QVector3D m_target;
    QVector3D m_up;

    float m_distance;
    float m_azimuth;
    float m_elevation;
    float m_fov;
    float m_nearPlane;
    float m_farPlane;

    void updatePosition();
};

/**
 * @brief Measurement tool for point clouds
 */
class MeasurementTool {
public:
    enum MeasurementType {
        Distance,       // Point-to-point distance
        Area,           // Polygon area
        Volume,         // Volume calculation
        Angle           // Angle between three points
    };

    struct Measurement {
        MeasurementType type;
        QVector<QVector3D> points;
        double value;
        QString label;
    };

    void addPoint(const QVector3D& point);
    void clear();
    Measurement getMeasurement(MeasurementType type);

private:
    QVector<QVector3D> m_points;

    double calculateDistance();
    double calculateArea();
    double calculateVolume();
    double calculateAngle();
};

/**
 * @brief 3D Point cloud viewer widget
 *
 * Features:
 * - PLY/LAS/XYZ file loading
 * - Efficient rendering of millions of points
 * - Octree spatial indexing for LOD
 * - Multiple color mapping schemes
 * - Interactive camera control
 * - Point filtering by intensity/classification
 * - Measurement tools (distance, area, volume)
 * - Normal visualization
 * - Export to various formats
 * - Integration with photogrammetry pipeline
 *
 * Supported formats:
 * - PLY (Polygon File Format)
 * - LAS/LAZ (LiDAR data)
 * - XYZ (ASCII point list)
 * - PCD (Point Cloud Data)
 *
 * Usage:
 *   PointCloudViewer* viewer = new PointCloudViewer(parent);
 *   viewer->loadPointCloud("reconstruction.ply");
 *   viewer->setColorScheme(PointCloudColorMap::Height);
 */
class PointCloudViewer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit PointCloudViewer(QWidget* parent = nullptr);
    ~PointCloudViewer() override;

    /**
     * @brief Load point cloud from file
     * @param filePath Path to point cloud file
     * @return True if loaded successfully
     */
    bool loadPointCloud(const QString& filePath);

    /**
     * @brief Set point cloud data directly
     * @param cloud Point cloud data
     */
    void setPointCloud(const PointCloud& cloud);

    /**
     * @brief Get current point cloud
     * @return Point cloud
     */
    const PointCloud& pointCloud() const { return m_cloud; }

    /**
     * @brief Set rendering settings
     * @param settings Rendering settings
     */
    void setSettings(const PointCloudSettings& settings);

    /**
     * @brief Get current settings
     * @return Current settings
     */
    PointCloudSettings settings() const { return m_settings; }

    /**
     * @brief Set color mapping scheme
     * @param scheme Color scheme
     */
    void setColorScheme(PointCloudColorMap::Scheme scheme);

    /**
     * @brief Get camera controller
     * @return Camera
     */
    PointCloudCamera& camera() { return m_camera; }

    /**
     * @brief Export point cloud to file
     * @param filePath Output file path
     * @return True if exported successfully
     */
    bool exportPointCloud(const QString& filePath);

    /**
     * @brief Export current view to image
     * @param filePath Output image path
     * @return True if exported successfully
     */
    bool exportImage(const QString& filePath);

    /**
     * @brief Get point at screen position
     * @param screenPos Screen position
     * @return Point index (-1 if none)
     */
    int getPointAt(const QPoint& screenPos);

    /**
     * @brief Enable measurement mode
     * @param enabled Enable state
     * @param type Measurement type
     */
    void enableMeasurement(bool enabled, MeasurementTool::MeasurementType type);

    /**
     * @brief Get statistics
     * @return Point cloud statistics string
     */
    QString getStatistics() const;

signals:
    void pointCloudLoaded(const QString& filePath, int numPoints);
    void renderingError(const QString& error);
    void pointSelected(int pointIndex);
    void measurementCompleted(double value, const QString& label);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    PointCloud m_cloud;
    PointCloudSettings m_settings;
    PointCloudCamera m_camera;
    PointCloudColorMap::Scheme m_colorScheme;
    Octree m_octree;
    MeasurementTool m_measurement;

    bool m_measurementMode;
    MeasurementTool::MeasurementType m_measurementType;

    // Mouse interaction
    QPoint m_lastMousePos;
    bool m_leftButtonPressed;
    bool m_middleButtonPressed;
    bool m_rightButtonPressed;

    // Rendering
    void renderPoints();
    void renderNormals();
    void renderMeasurements();
    void renderBoundingBox();

    // Color mapping
    QColor getPointColor(const Point& point);

    // Point picking
    int pickPoint(const QPoint& screenPos);
    QVector3D screenToWorld(const QPoint& screenPos, float depth);

    // LOD rendering
    QVector<int> getVisiblePoints();
};

} // namespace UI
} // namespace DroneMapper

#endif // POINTCLOUDVIEWER_H
