#ifndef TERRAINELEVATIONVIEWER_H
#define TERRAINELEVATIONVIEWER_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QImage>
#include "models/GeospatialCoordinate.h"
#include "models/FlightPlan.h"

namespace DroneMapper {
namespace UI {

/**
 * @brief Digital Elevation Model (DEM) data
 */
struct DEMData {
    int width;              // Number of samples in X
    int height;             // Number of samples in Y
    QVector<float> elevations;  // Height values (meters)

    Models::GeospatialCoordinate topLeft;
    Models::GeospatialCoordinate bottomRight;

    double resolution;      // Meters per pixel

    float minElevation;
    float maxElevation;

    /**
     * @brief Get elevation at pixel coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @return Elevation in meters
     */
    float getElevation(int x, int y) const;

    /**
     * @brief Get elevation at geographic coordinate
     * @param coord Geographic coordinate
     * @return Elevation in meters
     */
    float getElevationAt(const Models::GeospatialCoordinate& coord) const;

    /**
     * @brief Check if coordinate is within DEM bounds
     * @param coord Coordinate to check
     * @return True if within bounds
     */
    bool contains(const Models::GeospatialCoordinate& coord) const;

    /**
     * @brief Load from GeoTIFF file
     * @param filePath Path to GeoTIFF
     * @return True if loaded successfully
     */
    bool loadFromGeoTIFF(const QString& filePath);

    /**
     * @brief Load from SRTM file
     * @param filePath Path to SRTM file
     * @return True if loaded successfully
     */
    bool loadFromSRTM(const QString& filePath);

    /**
     * @brief Generate test terrain
     * @param width Width in samples
     * @param height Height in samples
     */
    void generateTestTerrain(int width, int height);
};

/**
 * @brief Terrain mesh vertex
 */
struct TerrainVertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
    QVector3D color;
};

/**
 * @brief Terrain visualization settings
 */
struct TerrainSettings {
    bool showTerrain;
    bool showWireframe;
    bool showContours;
    bool showFlightPath;
    bool showAltitudeProfile;

    double verticalExaggeration;  // 1.0 = realistic
    double contourInterval;       // Meters between contours

    bool colorByElevation;
    bool enableLighting;
    bool enableShadows;

    QColor wireframeColor;
    QColor contourColor;
    QColor flightPathColor;

    TerrainSettings();
};

/**
 * @brief Elevation color scheme
 */
class ElevationColorScheme {
public:
    enum Scheme {
        Terrain,        // Brown → Green → White (realistic)
        Hypsometric,    // Green → Yellow → Brown → White
        Atlas,          // Colorful atlas style
        Grayscale,      // Black → White
        Rainbow         // Full spectrum
    };

    /**
     * @brief Get color for elevation
     * @param elevation Elevation in meters
     * @param minElevation Minimum elevation
     * @param maxElevation Maximum elevation
     * @param scheme Color scheme
     * @return RGB color
     */
    static QVector3D getColor(
        float elevation,
        float minElevation,
        float maxElevation,
        Scheme scheme = Terrain);

private:
    static QVector3D interpolateColor(
        const QVector3D& color1,
        const QVector3D& color2,
        float t);
};

/**
 * @brief Camera controller for 3D view
 */
class TerrainCamera {
public:
    TerrainCamera();

    /**
     * @brief Set camera position
     * @param position Position in world space
     */
    void setPosition(const QVector3D& position);

    /**
     * @brief Set look-at target
     * @param target Target point
     */
    void setTarget(const QVector3D& target);

    /**
     * @brief Orbit around target
     * @param deltaAzimuth Azimuth change (degrees)
     * @param deltaElevation Elevation change (degrees)
     */
    void orbit(float deltaAzimuth, float deltaElevation);

    /**
     * @brief Zoom in/out
     * @param delta Zoom delta (positive = closer)
     */
    void zoom(float delta);

    /**
     * @brief Pan camera
     * @param deltaX X delta in screen space
     * @param deltaY Y delta in screen space
     */
    void pan(float deltaX, float deltaY);

    /**
     * @brief Get view matrix
     * @return View matrix
     */
    QMatrix4x4 viewMatrix() const;

    /**
     * @brief Get projection matrix
     * @param aspect Aspect ratio
     * @return Projection matrix
     */
    QMatrix4x4 projectionMatrix(float aspect) const;

    /**
     * @brief Reset camera to default position
     */
    void reset();

private:
    QVector3D m_position;
    QVector3D m_target;
    QVector3D m_up;

    float m_distance;
    float m_azimuth;     // Horizontal angle
    float m_elevation;   // Vertical angle

    float m_fov;         // Field of view (degrees)
    float m_nearPlane;
    float m_farPlane;

    void updatePosition();
};

/**
 * @brief 3D Terrain elevation viewer widget
 *
 * Features:
 * - DEM/SRTM terrain data visualization
 * - Real-time 3D rendering with OpenGL
 * - Elevation color mapping
 * - Contour line generation
 * - Flight path overlay in 3D
 * - Altitude profile display
 * - Interactive camera control (orbit, pan, zoom)
 * - Vertical exaggeration
 * - Lighting and shadow support
 * - Measurement tools
 * - Export to image/video
 *
 * Supported data formats:
 * - GeoTIFF
 * - SRTM (Shuttle Radar Topography Mission)
 * - ASTER GDEM
 * - ASCII Grid
 *
 * Usage:
 *   TerrainElevationViewer* viewer = new TerrainElevationViewer(parent);
 *   viewer->loadDEM("terrain.tif");
 *   viewer->setFlightPlan(flightPlan);
 */
class TerrainElevationViewer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit TerrainElevationViewer(QWidget* parent = nullptr);
    ~TerrainElevationViewer() override;

    /**
     * @brief Load DEM data
     * @param filePath Path to DEM file
     * @return True if loaded successfully
     */
    bool loadDEM(const QString& filePath);

    /**
     * @brief Set DEM data directly
     * @param data DEM data
     */
    void setDEMData(const DEMData& data);

    /**
     * @brief Set flight plan to overlay
     * @param plan Flight plan
     */
    void setFlightPlan(const Models::FlightPlan& plan);

    /**
     * @brief Set visualization settings
     * @param settings Terrain settings
     */
    void setSettings(const TerrainSettings& settings);

    /**
     * @brief Get current settings
     * @return Current settings
     */
    TerrainSettings settings() const { return m_settings; }

    /**
     * @brief Set elevation color scheme
     * @param scheme Color scheme
     */
    void setColorScheme(ElevationColorScheme::Scheme scheme);

    /**
     * @brief Get camera controller
     * @return Camera controller
     */
    TerrainCamera& camera() { return m_camera; }

    /**
     * @brief Export current view to image
     * @param filePath Output file path
     * @return True if exported successfully
     */
    bool exportImage(const QString& filePath);

    /**
     * @brief Get elevation at screen position
     * @param screenPos Screen position
     * @return Elevation in meters
     */
    float getElevationAt(const QPoint& screenPos);

    /**
     * @brief Measure distance between two points
     * @param start Start screen position
     * @param end End screen position
     * @return Distance in meters
     */
    double measureDistance(const QPoint& start, const QPoint& end);

signals:
    void demLoaded(const QString& filePath);
    void renderingError(const QString& error);
    void elevationQueried(float elevation);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    DEMData m_demData;
    Models::FlightPlan m_flightPlan;
    TerrainSettings m_settings;
    TerrainCamera m_camera;
    ElevationColorScheme::Scheme m_colorScheme;

    // OpenGL resources
    QVector<TerrainVertex> m_vertices;
    QVector<unsigned int> m_indices;

    // Mesh generation
    void generateTerrainMesh();
    void generateTerrainGrid();
    QVector3D calculateNormal(int x, int y);

    // Rendering
    void renderTerrain();
    void renderWireframe();
    void renderContours();
    void renderFlightPath();
    void renderAltitudeProfile();

    // Contour generation
    QVector<QVector<QVector3D>> generateContours(float interval);
    QVector<QVector3D> traceContour(float elevation);

    // Helper methods
    QVector3D demToWorld(int x, int y, float elevation) const;
    QVector3D geoToWorld(const Models::GeospatialCoordinate& coord) const;

    // Mouse interaction
    QPoint m_lastMousePos;
    bool m_leftButtonPressed;
    bool m_middleButtonPressed;
    bool m_rightButtonPressed;
};

/**
 * @brief Altitude profile widget
 *
 * Displays 2D altitude profile along flight path
 */
class AltitudeProfileWidget : public QWidget {
    Q_OBJECT

public:
    explicit AltitudeProfileWidget(QWidget* parent = nullptr);

    /**
     * @brief Set flight plan
     * @param plan Flight plan
     */
    void setFlightPlan(const Models::FlightPlan& plan);

    /**
     * @brief Set DEM data for terrain elevation
     * @param data DEM data
     */
    void setDEMData(const DEMData& data);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Models::FlightPlan m_flightPlan;
    DEMData m_demData;

    struct ProfilePoint {
        double distance;        // Distance from start (meters)
        double flightAltitude;  // Flight altitude AGL (meters)
        double terrainElevation;// Terrain elevation MSL (meters)
    };

    QVector<ProfilePoint> calculateProfile();
};

} // namespace UI
} // namespace DroneMapper

#endif // TERRAINELEVATIONVIEWER_H
