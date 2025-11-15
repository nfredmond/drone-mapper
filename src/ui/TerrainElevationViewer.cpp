#include "TerrainElevationViewer.h"
#include "geospatial/GeoUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <QFileInfo>
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace UI {

// DEMData implementation

float DEMData::getElevation(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0.0f;
    }

    int index = y * width + x;
    if (index >= 0 && index < elevations.size()) {
        return elevations[index];
    }

    return 0.0f;
}

float DEMData::getElevationAt(const Models::GeospatialCoordinate& coord) const
{
    if (!contains(coord)) {
        return 0.0f;
    }

    // Convert geographic coordinate to grid coordinates
    double dx = coord.longitude() - topLeft.longitude();
    double dy = topLeft.latitude() - coord.latitude();

    double totalDx = bottomRight.longitude() - topLeft.longitude();
    double totalDy = topLeft.latitude() - bottomRight.latitude();

    double x = (dx / totalDx) * (width - 1);
    double y = (dy / totalDy) * (height - 1);

    // Bilinear interpolation
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);

    float fx = x - x0;
    float fy = y - y0;

    float e00 = getElevation(x0, y0);
    float e10 = getElevation(x1, y0);
    float e01 = getElevation(x0, y1);
    float e11 = getElevation(x1, y1);

    float e0 = e00 * (1 - fx) + e10 * fx;
    float e1 = e01 * (1 - fx) + e11 * fx;

    return e0 * (1 - fy) + e1 * fy;
}

bool DEMData::contains(const Models::GeospatialCoordinate& coord) const
{
    return coord.latitude() <= topLeft.latitude() &&
           coord.latitude() >= bottomRight.latitude() &&
           coord.longitude() >= topLeft.longitude() &&
           coord.longitude() <= bottomRight.longitude();
}

bool DEMData::loadFromGeoTIFF(const QString& filePath)
{
    Q_UNUSED(filePath);
    // Would use GDAL to load GeoTIFF in production
    // For now, generate test terrain
    generateTestTerrain(128, 128);
    return true;
}

bool DEMData::loadFromSRTM(const QString& filePath)
{
    Q_UNUSED(filePath);
    // Would load SRTM HGT file in production
    generateTestTerrain(128, 128);
    return true;
}

void DEMData::generateTestTerrain(int w, int h)
{
    width = w;
    height = h;
    elevations.resize(width * height);

    minElevation = 0.0f;
    maxElevation = 500.0f;

    // Generate terrain using multiple octaves of Perlin-like noise
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float nx = static_cast<float>(x) / width;
            float ny = static_cast<float>(y) / height;

            // Multiple octaves for terrain-like features
            float elevation = 0.0f;
            float amplitude = 200.0f;
            float frequency = 1.0f;

            for (int octave = 0; octave < 4; ++octave) {
                elevation += amplitude * (sin(nx * frequency * 10 + cos(ny * frequency * 8)) +
                                        cos(ny * frequency * 10 + sin(nx * frequency * 12))) * 0.5f;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            elevation = std::max(0.0f, elevation + 100.0f);

            int index = y * width + x;
            elevations[index] = elevation;
        }
    }

    // Set bounds (example: 1km x 1km area)
    topLeft = Models::GeospatialCoordinate(47.0, -122.0, 0);
    bottomRight = Models::GeospatialCoordinate(46.99, -121.99, 0);
    resolution = 10.0; // 10 meters per pixel
}

// TerrainSettings implementation

TerrainSettings::TerrainSettings()
    : showTerrain(true)
    , showWireframe(false)
    , showContours(true)
    , showFlightPath(true)
    , showAltitudeProfile(false)
    , verticalExaggeration(2.0)
    , contourInterval(50.0)
    , colorByElevation(true)
    , enableLighting(true)
    , enableShadows(false)
    , wireframeColor(Qt::white)
    , contourColor(100, 100, 100, 200)
    , flightPathColor(Qt::red)
{
}

// ElevationColorScheme implementation

QVector3D ElevationColorScheme::getColor(
    float elevation,
    float minElevation,
    float maxElevation,
    Scheme scheme)
{
    float normalized = (elevation - minElevation) / (maxElevation - minElevation);
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    switch (scheme) {
    case Terrain:
        // Brown (low) → Green → Gray → White (high)
        if (normalized < 0.3f) {
            return interpolateColor(
                QVector3D(0.6f, 0.4f, 0.2f),  // Brown
                QVector3D(0.2f, 0.6f, 0.2f),  // Green
                normalized / 0.3f);
        } else if (normalized < 0.7f) {
            return interpolateColor(
                QVector3D(0.2f, 0.6f, 0.2f),  // Green
                QVector3D(0.5f, 0.5f, 0.5f),  // Gray
                (normalized - 0.3f) / 0.4f);
        } else {
            return interpolateColor(
                QVector3D(0.5f, 0.5f, 0.5f),  // Gray
                QVector3D(1.0f, 1.0f, 1.0f),  // White
                (normalized - 0.7f) / 0.3f);
        }

    case Hypsometric:
        // Green → Yellow → Brown → White
        if (normalized < 0.25f) {
            return interpolateColor(
                QVector3D(0.0f, 0.5f, 0.0f),
                QVector3D(1.0f, 1.0f, 0.0f),
                normalized / 0.25f);
        } else if (normalized < 0.75f) {
            return interpolateColor(
                QVector3D(1.0f, 1.0f, 0.0f),
                QVector3D(0.6f, 0.3f, 0.0f),
                (normalized - 0.25f) / 0.5f);
        } else {
            return interpolateColor(
                QVector3D(0.6f, 0.3f, 0.0f),
                QVector3D(1.0f, 1.0f, 1.0f),
                (normalized - 0.75f) / 0.25f);
        }

    case Grayscale:
        return QVector3D(normalized, normalized, normalized);

    case Rainbow:
        // Blue → Cyan → Green → Yellow → Red
        if (normalized < 0.25f) {
            return interpolateColor(
                QVector3D(0.0f, 0.0f, 1.0f),
                QVector3D(0.0f, 1.0f, 1.0f),
                normalized / 0.25f);
        } else if (normalized < 0.5f) {
            return interpolateColor(
                QVector3D(0.0f, 1.0f, 1.0f),
                QVector3D(0.0f, 1.0f, 0.0f),
                (normalized - 0.25f) / 0.25f);
        } else if (normalized < 0.75f) {
            return interpolateColor(
                QVector3D(0.0f, 1.0f, 0.0f),
                QVector3D(1.0f, 1.0f, 0.0f),
                (normalized - 0.5f) / 0.25f);
        } else {
            return interpolateColor(
                QVector3D(1.0f, 1.0f, 0.0f),
                QVector3D(1.0f, 0.0f, 0.0f),
                (normalized - 0.75f) / 0.25f);
        }

    default:
        return QVector3D(normalized, normalized, normalized);
    }
}

QVector3D ElevationColorScheme::interpolateColor(
    const QVector3D& color1,
    const QVector3D& color2,
    float t)
{
    return color1 * (1.0f - t) + color2 * t;
}

// TerrainCamera implementation

TerrainCamera::TerrainCamera()
    : m_target(0, 0, 0)
    , m_up(0, 0, 1)
    , m_distance(1000.0f)
    , m_azimuth(45.0f)
    , m_elevation(30.0f)
    , m_fov(45.0f)
    , m_nearPlane(1.0f)
    , m_farPlane(10000.0f)
{
    updatePosition();
}

void TerrainCamera::setPosition(const QVector3D& position)
{
    m_position = position;
}

void TerrainCamera::setTarget(const QVector3D& target)
{
    m_target = target;
    updatePosition();
}

void TerrainCamera::orbit(float deltaAzimuth, float deltaElevation)
{
    m_azimuth += deltaAzimuth;
    m_elevation = std::clamp(m_elevation + deltaElevation, -89.0f, 89.0f);
    updatePosition();
}

void TerrainCamera::zoom(float delta)
{
    m_distance = std::clamp(m_distance - delta, 10.0f, 10000.0f);
    updatePosition();
}

void TerrainCamera::pan(float deltaX, float deltaY)
{
    QVector3D right = QVector3D::crossProduct(m_position - m_target, m_up).normalized();
    QVector3D up = QVector3D::crossProduct(right, m_position - m_target).normalized();

    m_target += right * deltaX + up * deltaY;
    updatePosition();
}

QMatrix4x4 TerrainCamera::viewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(m_position, m_target, m_up);
    return view;
}

QMatrix4x4 TerrainCamera::projectionMatrix(float aspect) const
{
    QMatrix4x4 projection;
    projection.perspective(m_fov, aspect, m_nearPlane, m_farPlane);
    return projection;
}

void TerrainCamera::reset()
{
    m_target = QVector3D(0, 0, 0);
    m_distance = 1000.0f;
    m_azimuth = 45.0f;
    m_elevation = 30.0f;
    updatePosition();
}

void TerrainCamera::updatePosition()
{
    float azimuthRad = qDegreesToRadians(m_azimuth);
    float elevationRad = qDegreesToRadians(m_elevation);

    float x = m_distance * cos(elevationRad) * cos(azimuthRad);
    float y = m_distance * cos(elevationRad) * sin(azimuthRad);
    float z = m_distance * sin(elevationRad);

    m_position = m_target + QVector3D(x, y, z);
}

// TerrainElevationViewer implementation

TerrainElevationViewer::TerrainElevationViewer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_colorScheme(ElevationColorScheme::Terrain)
    , m_leftButtonPressed(false)
    , m_middleButtonPressed(false)
    , m_rightButtonPressed(false)
{
    setFocusPolicy(Qt::StrongFocus);
}

TerrainElevationViewer::~TerrainElevationViewer()
{
    makeCurrent();
    // Cleanup OpenGL resources
    doneCurrent();
}

bool TerrainElevationViewer::loadDEM(const QString& filePath)
{
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        emit renderingError("DEM file not found: " + filePath);
        return false;
    }

    QString extension = fileInfo.suffix().toLower();

    bool success = false;
    if (extension == "tif" || extension == "tiff") {
        success = m_demData.loadFromGeoTIFF(filePath);
    } else if (extension == "hgt") {
        success = m_demData.loadFromSRTM(filePath);
    } else {
        emit renderingError("Unsupported file format: " + extension);
        return false;
    }

    if (success) {
        generateTerrainMesh();
        update();
        emit demLoaded(filePath);
    }

    return success;
}

void TerrainElevationViewer::setDEMData(const DEMData& data)
{
    m_demData = data;
    generateTerrainMesh();
    update();
}

void TerrainElevationViewer::setFlightPlan(const Models::FlightPlan& plan)
{
    m_flightPlan = plan;
    update();
}

void TerrainElevationViewer::setSettings(const TerrainSettings& settings)
{
    m_settings = settings;
    update();
}

void TerrainElevationViewer::setColorScheme(ElevationColorScheme::Scheme scheme)
{
    m_colorScheme = scheme;
    generateTerrainMesh(); // Regenerate with new colors
    update();
}

bool TerrainElevationViewer::exportImage(const QString& filePath)
{
    QImage image = grabFramebuffer();
    return image.save(filePath);
}

float TerrainElevationViewer::getElevationAt(const QPoint& screenPos)
{
    Q_UNUSED(screenPos);
    // Would implement ray-terrain intersection in production
    return 0.0f;
}

double TerrainElevationViewer::measureDistance(const QPoint& start, const QPoint& end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    // Would implement 3D distance measurement in production
    return 0.0;
}

void TerrainElevationViewer::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);  // Sky blue background
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Generate test terrain if no DEM loaded
    if (m_demData.elevations.isEmpty()) {
        m_demData.generateTestTerrain(128, 128);
        generateTerrainMesh();
    }
}

void TerrainElevationViewer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void TerrainElevationViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_vertices.isEmpty()) {
        return;
    }

    // Setup matrices
    float aspect = static_cast<float>(width()) / height();
    QMatrix4x4 projection = m_camera.projectionMatrix(aspect);
    QMatrix4x4 view = m_camera.viewMatrix();
    QMatrix4x4 model;

    // Render terrain
    if (m_settings.showTerrain) {
        renderTerrain();
    }

    // Render wireframe
    if (m_settings.showWireframe) {
        renderWireframe();
    }

    // Render contours
    if (m_settings.showContours) {
        renderContours();
    }

    // Render flight path
    if (m_settings.showFlightPath && !m_flightPlan.waypoints().isEmpty()) {
        renderFlightPath();
    }
}

void TerrainElevationViewer::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton) {
        m_leftButtonPressed = true;
    } else if (event->button() == Qt::MiddleButton) {
        m_middleButtonPressed = true;
    } else if (event->button() == Qt::RightButton) {
        m_rightButtonPressed = true;
    }
}

void TerrainElevationViewer::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_lastMousePos;

    if (m_leftButtonPressed) {
        // Orbit camera
        m_camera.orbit(delta.x() * 0.5f, -delta.y() * 0.5f);
        update();
    } else if (m_middleButtonPressed || m_rightButtonPressed) {
        // Pan camera
        m_camera.pan(-delta.x() * 2.0f, delta.y() * 2.0f);
        update();
    }

    m_lastMousePos = event->pos();
}

void TerrainElevationViewer::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() * 0.5f;
    m_camera.zoom(delta);
    update();
}

void TerrainElevationViewer::generateTerrainMesh()
{
    if (m_demData.elevations.isEmpty()) {
        return;
    }

    m_vertices.clear();
    m_indices.clear();

    int width = m_demData.width;
    int height = m_demData.height;

    // Generate vertices
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            TerrainVertex vertex;

            float elevation = m_demData.getElevation(x, y);
            vertex.position = demToWorld(x, y, elevation * m_settings.verticalExaggeration);
            vertex.normal = calculateNormal(x, y);
            vertex.texCoord = QVector2D(
                static_cast<float>(x) / (width - 1),
                static_cast<float>(y) / (height - 1));

            if (m_settings.colorByElevation) {
                vertex.color = ElevationColorScheme::getColor(
                    elevation,
                    m_demData.minElevation,
                    m_demData.maxElevation,
                    m_colorScheme);
            } else {
                vertex.color = QVector3D(0.5f, 0.5f, 0.5f);
            }

            m_vertices.append(vertex);
        }
    }

    // Generate indices for triangles
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            int i0 = y * width + x;
            int i1 = y * width + (x + 1);
            int i2 = (y + 1) * width + x;
            int i3 = (y + 1) * width + (x + 1);

            // First triangle
            m_indices.append(i0);
            m_indices.append(i2);
            m_indices.append(i1);

            // Second triangle
            m_indices.append(i1);
            m_indices.append(i2);
            m_indices.append(i3);
        }
    }
}

void TerrainElevationViewer::generateTerrainGrid()
{
    generateTerrainMesh();
}

QVector3D TerrainElevationViewer::calculateNormal(int x, int y)
{
    int width = m_demData.width;
    int height = m_demData.height;

    // Central differences for normal calculation
    float hL = x > 0 ? m_demData.getElevation(x - 1, y) : m_demData.getElevation(x, y);
    float hR = x < width - 1 ? m_demData.getElevation(x + 1, y) : m_demData.getElevation(x, y);
    float hD = y > 0 ? m_demData.getElevation(x, y - 1) : m_demData.getElevation(x, y);
    float hU = y < height - 1 ? m_demData.getElevation(x, y + 1) : m_demData.getElevation(x, y);

    QVector3D normal(hL - hR, hD - hU, 2.0f * m_demData.resolution);
    return normal.normalized();
}

void TerrainElevationViewer::renderTerrain()
{
    // Simplified rendering using immediate mode
    // Production would use VBOs/VAOs
    glBegin(GL_TRIANGLES);

    for (int i = 0; i < m_indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            const TerrainVertex& v = m_vertices[m_indices[i + j]];

            if (m_settings.enableLighting) {
                glNormal3f(v.normal.x(), v.normal.y(), v.normal.z());
            }

            glColor3f(v.color.x(), v.color.y(), v.color.z());
            glVertex3f(v.position.x(), v.position.y(), v.position.z());
        }
    }

    glEnd();
}

void TerrainElevationViewer::renderWireframe()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(1.0f, 1.0f, 1.0f);

    renderTerrain();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void TerrainElevationViewer::renderContours()
{
    // Simplified contour rendering
    // Production would use marching squares/cubes
}

void TerrainElevationViewer::renderFlightPath()
{
    glColor3f(1.0f, 0.0f, 0.0f);  // Red
    glLineWidth(3.0f);

    glBegin(GL_LINE_STRIP);

    for (const auto& waypoint : m_flightPlan.waypoints()) {
        QVector3D pos = geoToWorld(waypoint.coordinate());
        glVertex3f(pos.x(), pos.y(), pos.z());
    }

    glEnd();
}

void TerrainElevationViewer::renderAltitudeProfile()
{
    // Would render 2D altitude profile in production
}

QVector<QVector<QVector3D>> TerrainElevationViewer::generateContours(float interval)
{
    Q_UNUSED(interval);
    return QVector<QVector<QVector3D>>();
}

QVector<QVector3D> TerrainElevationViewer::traceContour(float elevation)
{
    Q_UNUSED(elevation);
    return QVector<QVector3D>();
}

QVector3D TerrainElevationViewer::demToWorld(int x, int y, float elevation) const
{
    // Simple grid to world transform
    float worldX = static_cast<float>(x) * m_demData.resolution;
    float worldY = static_cast<float>(y) * m_demData.resolution;
    float worldZ = elevation;

    return QVector3D(worldX, worldY, worldZ);
}

QVector3D TerrainElevationViewer::geoToWorld(const Models::GeospatialCoordinate& coord) const
{
    // Convert geographic to DEM grid, then to world
    if (!m_demData.contains(coord)) {
        return QVector3D(0, 0, 0);
    }

    double dx = coord.longitude() - m_demData.topLeft.longitude();
    double dy = m_demData.topLeft.latitude() - coord.latitude();

    double totalDx = m_demData.bottomRight.longitude() - m_demData.topLeft.longitude();
    double totalDy = m_demData.topLeft.latitude() - m_demData.bottomRight.latitude();

    float x = static_cast<float>((dx / totalDx) * (m_demData.width - 1));
    float y = static_cast<float>((dy / totalDy) * (m_demData.height - 1));

    float elevation = m_demData.getElevationAt(coord) + coord.altitude();

    return demToWorld(static_cast<int>(x), static_cast<int>(y),
                     elevation * m_settings.verticalExaggeration);
}

// AltitudeProfileWidget implementation

AltitudeProfileWidget::AltitudeProfileWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(150);
}

void AltitudeProfileWidget::setFlightPlan(const Models::FlightPlan& plan)
{
    m_flightPlan = plan;
    update();
}

void AltitudeProfileWidget::setDEMData(const DEMData& data)
{
    m_demData = data;
    update();
}

void AltitudeProfileWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_flightPlan.waypoints().isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, "No flight plan loaded");
        return;
    }

    QVector<ProfilePoint> profile = calculateProfile();

    if (profile.isEmpty()) {
        return;
    }

    // Draw profile
    QRectF plotArea = rect().adjusted(40, 20, -20, -40);

    // Find min/max for scaling
    double maxDist = profile.last().distance;
    double maxAlt = 0;
    double minAlt = profile.first().terrainElevation;

    for (const auto& point : profile) {
        maxAlt = std::max(maxAlt, point.flightAltitude + point.terrainElevation);
        minAlt = std::min(minAlt, point.terrainElevation);
    }

    double altRange = maxAlt - minAlt + 20; // Add 20m margin

    // Draw terrain
    QPainterPath terrainPath;
    terrainPath.moveTo(
        plotArea.left(),
        plotArea.bottom() - ((profile[0].terrainElevation - minAlt) / altRange) * plotArea.height());

    for (const auto& point : profile) {
        double x = plotArea.left() + (point.distance / maxDist) * plotArea.width();
        double y = plotArea.bottom() - ((point.terrainElevation - minAlt) / altRange) * plotArea.height();
        terrainPath.lineTo(x, y);
    }

    terrainPath.lineTo(plotArea.right(), plotArea.bottom());
    terrainPath.lineTo(plotArea.left(), plotArea.bottom());

    painter.fillPath(terrainPath, QColor(150, 100, 50, 150));
    painter.setPen(QPen(QColor(100, 70, 30), 2));
    painter.drawPath(terrainPath);

    // Draw flight path
    painter.setPen(QPen(Qt::red, 2));
    for (int i = 0; i < profile.size() - 1; ++i) {
        double x1 = plotArea.left() + (profile[i].distance / maxDist) * plotArea.width();
        double y1 = plotArea.bottom() - ((profile[i].flightAltitude + profile[i].terrainElevation - minAlt) / altRange) * plotArea.height();

        double x2 = plotArea.left() + (profile[i + 1].distance / maxDist) * plotArea.width();
        double y2 = plotArea.bottom() - ((profile[i + 1].flightAltitude + profile[i + 1].terrainElevation - minAlt) / altRange) * plotArea.height();

        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    // Draw axes
    painter.setPen(Qt::black);
    painter.drawRect(plotArea);
    painter.drawText(plotArea.adjusted(0, plotArea.height() + 5, 0, 0),
                    Qt::AlignCenter,
                    QString("Distance: %1 m").arg(maxDist, 0, 'f', 0));
}

QVector<AltitudeProfileWidget::ProfilePoint> AltitudeProfileWidget::calculateProfile()
{
    QVector<ProfilePoint> profile;

    double cumulativeDistance = 0.0;

    for (int i = 0; i < m_flightPlan.waypoints().size(); ++i) {
        const auto& waypoint = m_flightPlan.waypoints()[i];

        ProfilePoint point;
        point.distance = cumulativeDistance;
        point.flightAltitude = waypoint.coordinate().altitude();

        if (!m_demData.elevations.isEmpty()) {
            point.terrainElevation = m_demData.getElevationAt(waypoint.coordinate());
        } else {
            point.terrainElevation = 0;
        }

        profile.append(point);

        // Calculate distance to next waypoint
        if (i < m_flightPlan.waypoints().size() - 1) {
            const auto& nextWaypoint = m_flightPlan.waypoints()[i + 1];
            cumulativeDistance += Geospatial::GeoUtils::distanceBetween(
                waypoint.coordinate(),
                nextWaypoint.coordinate());
        }
    }

    return profile;
}

} // namespace UI
} // namespace DroneMapper
