#include "PointCloudViewer.h"
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QFileInfo>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace DroneMapper {
namespace UI {

// PointCloud implementation

void PointCloud::clear()
{
    points.clear();
    fileName.clear();
    hasColors = false;
    hasNormals = false;
    hasIntensity = false;
    hasClassification = false;
}

void PointCloud::calculateBounds()
{
    if (points.isEmpty()) {
        minBounds = QVector3D(0, 0, 0);
        maxBounds = QVector3D(0, 0, 0);
        return;
    }

    minBounds = points.first().position;
    maxBounds = points.first().position;

    for (const auto& point : points) {
        minBounds.setX(std::min(minBounds.x(), point.position.x()));
        minBounds.setY(std::min(minBounds.y(), point.position.y()));
        minBounds.setZ(std::min(minBounds.z(), point.position.z()));

        maxBounds.setX(std::max(maxBounds.x(), point.position.x()));
        maxBounds.setY(std::max(maxBounds.y(), point.position.y()));
        maxBounds.setZ(std::max(maxBounds.z(), point.position.z()));
    }
}

void PointCloud::calculateCentroid()
{
    if (points.isEmpty()) {
        centroid = QVector3D(0, 0, 0);
        return;
    }

    QVector3D sum(0, 0, 0);
    for (const auto& point : points) {
        sum += point.position;
    }

    centroid = sum / points.size();
}

bool PointCloud::loadFromPLY(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    clear();
    fileName = filePath;

    QTextStream in(&file);

    // Parse PLY header
    QString line = in.readLine();
    if (line != "ply") {
        return false;
    }

    int vertexCount = 0;
    bool hasColor = false;
    bool hasNormal = false;
    bool inHeader = true;

    while (!in.atEnd() && inHeader) {
        line = in.readLine().trimmed();

        if (line.startsWith("element vertex")) {
            QStringList parts = line.split(' ');
            if (parts.size() >= 3) {
                vertexCount = parts[2].toInt();
            }
        } else if (line.contains("red") || line.contains("green") || line.contains("blue")) {
            hasColor = true;
        } else if (line.contains("nx") || line.contains("ny") || line.contains("nz")) {
            hasNormal = true;
        } else if (line == "end_header") {
            inHeader = false;
        }
    }

    hasColors = hasColor;
    hasNormals = hasNormal;

    // Read vertex data
    points.reserve(vertexCount);

    for (int i = 0; i < vertexCount && !in.atEnd(); ++i) {
        line = in.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);

        if (parts.size() < 3) {
            continue;
        }

        Point point;
        point.position.setX(parts[0].toFloat());
        point.position.setY(parts[1].toFloat());
        point.position.setZ(parts[2].toFloat());

        int offset = 3;

        if (hasNormal && parts.size() >= offset + 3) {
            point.normal.setX(parts[offset].toFloat());
            point.normal.setY(parts[offset + 1].toFloat());
            point.normal.setZ(parts[offset + 2].toFloat());
            offset += 3;
        }

        if (hasColor && parts.size() >= offset + 3) {
            int r = parts[offset].toInt();
            int g = parts[offset + 1].toInt();
            int b = parts[offset + 2].toInt();
            point.color = QColor(r, g, b);
        }

        points.append(point);
    }

    file.close();

    calculateBounds();
    calculateCentroid();

    return true;
}

bool PointCloud::loadFromLAS(const QString& filePath)
{
    Q_UNUSED(filePath);
    // Would use libLAS or PDAL in production
    // For now, generate test cloud
    generateTestCloud(5000);
    return true;
}

bool PointCloud::loadFromXYZ(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    clear();
    fileName = filePath;

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 3) {
            continue;
        }

        Point point;
        point.position.setX(parts[0].toFloat());
        point.position.setY(parts[1].toFloat());
        point.position.setZ(parts[2].toFloat());

        if (parts.size() >= 6) {
            int r = parts[3].toInt();
            int g = parts[4].toInt();
            int b = parts[5].toInt();
            point.color = QColor(r, g, b);
            hasColors = true;
        }

        points.append(point);
    }

    file.close();

    calculateBounds();
    calculateCentroid();

    return true;
}

bool PointCloud::saveToPLY(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // Write PLY header
    out << "ply\n";
    out << "format ascii 1.0\n";
    out << "element vertex " << points.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";

    if (hasNormals) {
        out << "property float nx\n";
        out << "property float ny\n";
        out << "property float nz\n";
    }

    if (hasColors) {
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
    }

    out << "end_header\n";

    // Write vertex data
    for (const auto& point : points) {
        out << point.position.x() << " "
            << point.position.y() << " "
            << point.position.z();

        if (hasNormals) {
            out << " " << point.normal.x()
                << " " << point.normal.y()
                << " " << point.normal.z();
        }

        if (hasColors) {
            out << " " << point.color.red()
                << " " << point.color.green()
                << " " << point.color.blue();
        }

        out << "\n";
    }

    file.close();
    return true;
}

void PointCloud::generateTestCloud(int numPoints)
{
    clear();

    points.reserve(numPoints);
    hasColors = true;
    hasNormals = false;

    // Generate sphere of points
    for (int i = 0; i < numPoints; ++i) {
        Point point;

        // Random spherical coordinates
        float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
        float phi = static_cast<float>(rand()) / RAND_MAX * M_PI;
        float radius = 100.0f + static_cast<float>(rand()) / RAND_MAX * 50.0f;

        point.position.setX(radius * sin(phi) * cos(theta));
        point.position.setY(radius * sin(phi) * sin(theta));
        point.position.setZ(radius * cos(phi));

        // Color based on position
        point.color = QColor(
            static_cast<int>((point.position.x() + 150) / 300 * 255),
            static_cast<int>((point.position.y() + 150) / 300 * 255),
            static_cast<int>((point.position.z() + 150) / 300 * 255));

        points.append(point);
    }

    calculateBounds();
    calculateCentroid();
}

// PointCloudSettings implementation

PointCloudSettings::PointCloudSettings()
    : showPoints(true)
    , pointSize(2.0f)
    , usePointColor(true)
    , useColorMap(false)
    , showNormals(false)
    , normalLength(1.0f)
    , enableLighting(false)
    , enableDepthTest(true)
    , defaultColor(Qt::white)
    , opacity(1.0f)
    , enableFiltering(false)
    , minIntensity(0.0f)
    , maxIntensity(1.0f)
    , enableLOD(true)
    , lodDistance1(100.0f)
    , lodDistance2(500.0f)
    , lodDistance3(1000.0f)
{
}

// PointCloudColorMap implementation

QColor PointCloudColorMap::getColorForHeight(float z, float minZ, float maxZ)
{
    float normalized = (z - minZ) / (maxZ - minZ);
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    // Blue (low) → Green → Yellow → Red (high)
    if (normalized < 0.25f) {
        float t = normalized / 0.25f;
        return QColor(
            static_cast<int>(t * 255),
            static_cast<int>(t * 255),
            255);
    } else if (normalized < 0.5f) {
        float t = (normalized - 0.25f) / 0.25f;
        return QColor(
            static_cast<int>(t * 255),
            255,
            static_cast<int>((1 - t) * 255));
    } else if (normalized < 0.75f) {
        float t = (normalized - 0.5f) / 0.25f;
        return QColor(
            255,
            255,
            static_cast<int>(t * 255));
    } else {
        float t = (normalized - 0.75f) / 0.25f;
        return QColor(
            255,
            static_cast<int>((1 - t) * 255),
            static_cast<int>((1 - t) * 255));
    }
}

QColor PointCloudColorMap::getColorForIntensity(float intensity)
{
    int gray = static_cast<int>(intensity * 255);
    return QColor(gray, gray, gray);
}

QColor PointCloudColorMap::getColorForClassification(int classification)
{
    // Standard LAS classification colors
    switch (classification) {
    case 0:  return QColor(0, 0, 0);           // Never classified
    case 1:  return QColor(128, 128, 128);     // Unclassified
    case 2:  return QColor(139, 69, 19);       // Ground
    case 3:  return QColor(0, 128, 0);         // Low vegetation
    case 4:  return QColor(0, 200, 0);         // Medium vegetation
    case 5:  return QColor(0, 255, 0);         // High vegetation
    case 6:  return QColor(255, 0, 0);         // Building
    case 7:  return QColor(128, 0, 255);       // Low point (noise)
    case 9:  return QColor(0, 0, 255);         // Water
    case 17: return QColor(255, 255, 0);       // Bridge deck
    default: return QColor(255, 255, 255);     // Unknown
    }
}

QColor PointCloudColorMap::getColorForNormal(const QVector3D& normal)
{
    // Map normal direction to RGB
    int r = static_cast<int>((normal.x() + 1.0f) * 127.5f);
    int g = static_cast<int>((normal.y() + 1.0f) * 127.5f);
    int b = static_cast<int>((normal.z() + 1.0f) * 127.5f);
    return QColor(r, g, b);
}

// Octree implementation

Octree::Octree()
    : m_root(nullptr)
    , m_cloud(nullptr)
{
}

Octree::~Octree()
{
    clear();
}

void Octree::build(const PointCloud& cloud, int maxPointsPerNode)
{
    clear();

    m_cloud = &cloud;

    if (cloud.isEmpty()) {
        return;
    }

    // Create root node
    m_root = new OctreeNode();
    m_root->center = (cloud.minBounds + cloud.maxBounds) * 0.5f;

    QVector3D size = cloud.maxBounds - cloud.minBounds;
    m_root->halfSize = std::max(std::max(size.x(), size.y()), size.z()) * 0.5f;

    // Build index list
    QVector<int> allIndices;
    allIndices.reserve(cloud.size());
    for (int i = 0; i < cloud.size(); ++i) {
        allIndices.append(i);
    }

    buildNode(m_root, allIndices, maxPointsPerNode);
}

void Octree::clear()
{
    if (m_root) {
        deleteNode(m_root);
        m_root = nullptr;
    }
    m_cloud = nullptr;
}

QVector<int> Octree::queryFrustum(const QVector<QVector4D>& frustumPlanes)
{
    QVector<int> result;

    if (!m_root) {
        return result;
    }

    queryNode(m_root, frustumPlanes, result);
    return result;
}

int Octree::getLODLevel(const OctreeNode* node, const QVector3D& cameraPos)
{
    float distance = (node->center - cameraPos).length();

    if (distance < 100.0f) return 0;      // Full detail
    if (distance < 500.0f) return 1;      // Medium detail
    if (distance < 1000.0f) return 2;     // Low detail
    return 3;                              // Very low detail
}

void Octree::buildNode(OctreeNode* node, const QVector<int>& indices, int maxPointsPerNode)
{
    if (indices.size() <= maxPointsPerNode) {
        // Leaf node
        node->pointIndices = indices;
        return;
    }

    // Subdivide
    node->children.resize(8);

    for (int i = 0; i < 8; ++i) {
        node->children[i] = new OctreeNode();

        // Calculate child center
        float offsetX = ((i & 1) ? 1.0f : -1.0f) * node->halfSize * 0.5f;
        float offsetY = ((i & 2) ? 1.0f : -1.0f) * node->halfSize * 0.5f;
        float offsetZ = ((i & 4) ? 1.0f : -1.0f) * node->halfSize * 0.5f;

        node->children[i]->center = node->center + QVector3D(offsetX, offsetY, offsetZ);
        node->children[i]->halfSize = node->halfSize * 0.5f;
    }

    // Distribute points to children
    QVector<QVector<int>> childIndices(8);

    for (int idx : indices) {
        const Point& point = m_cloud->points[idx];
        QVector3D pos = point.position;

        int childIdx = 0;
        if (pos.x() > node->center.x()) childIdx |= 1;
        if (pos.y() > node->center.y()) childIdx |= 2;
        if (pos.z() > node->center.z()) childIdx |= 4;

        childIndices[childIdx].append(idx);
    }

    // Recursively build children
    for (int i = 0; i < 8; ++i) {
        if (!childIndices[i].isEmpty()) {
            buildNode(node->children[i], childIndices[i], maxPointsPerNode);
        }
    }
}

void Octree::queryNode(OctreeNode* node, const QVector<QVector4D>& planes, QVector<int>& result)
{
    // Simplified frustum culling
    // In production would test AABB against all frustum planes

    if (node->isLeaf()) {
        result.append(node->pointIndices);
    } else {
        for (auto* child : node->children) {
            if (child) {
                queryNode(child, planes, result);
            }
        }
    }
}

void Octree::deleteNode(OctreeNode* node)
{
    if (!node) {
        return;
    }

    for (auto* child : node->children) {
        deleteNode(child);
    }

    delete node;
}

// PointCloudCamera implementation

PointCloudCamera::PointCloudCamera()
    : m_target(0, 0, 0)
    , m_up(0, 0, 1)
    , m_distance(500.0f)
    , m_azimuth(45.0f)
    , m_elevation(30.0f)
    , m_fov(45.0f)
    , m_nearPlane(1.0f)
    , m_farPlane(10000.0f)
{
    updatePosition();
}

void PointCloudCamera::setTarget(const QVector3D& target)
{
    m_target = target;
    updatePosition();
}

void PointCloudCamera::orbit(float deltaAzimuth, float deltaElevation)
{
    m_azimuth += deltaAzimuth;
    m_elevation = std::clamp(m_elevation + deltaElevation, -89.0f, 89.0f);
    updatePosition();
}

void PointCloudCamera::zoom(float delta)
{
    m_distance = std::clamp(m_distance - delta, 10.0f, 10000.0f);
    updatePosition();
}

void PointCloudCamera::pan(float deltaX, float deltaY)
{
    QVector3D right = QVector3D::crossProduct(m_position - m_target, m_up).normalized();
    QVector3D up = QVector3D::crossProduct(right, m_position - m_target).normalized();

    m_target += right * deltaX + up * deltaY;
    updatePosition();
}

void PointCloudCamera::reset()
{
    m_target = QVector3D(0, 0, 0);
    m_distance = 500.0f;
    m_azimuth = 45.0f;
    m_elevation = 30.0f;
    updatePosition();
}

QMatrix4x4 PointCloudCamera::viewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(m_position, m_target, m_up);
    return view;
}

QMatrix4x4 PointCloudCamera::projectionMatrix(float aspect) const
{
    QMatrix4x4 projection;
    projection.perspective(m_fov, aspect, m_nearPlane, m_farPlane);
    return projection;
}

void PointCloudCamera::updatePosition()
{
    float azimuthRad = qDegreesToRadians(m_azimuth);
    float elevationRad = qDegreesToRadians(m_elevation);

    float x = m_distance * cos(elevationRad) * cos(azimuthRad);
    float y = m_distance * cos(elevationRad) * sin(azimuthRad);
    float z = m_distance * sin(elevationRad);

    m_position = m_target + QVector3D(x, y, z);
}

// MeasurementTool implementation

void MeasurementTool::addPoint(const QVector3D& point)
{
    m_points.append(point);
}

void MeasurementTool::clear()
{
    m_points.clear();
}

MeasurementTool::Measurement MeasurementTool::getMeasurement(MeasurementType type)
{
    Measurement result;
    result.type = type;
    result.points = m_points;

    switch (type) {
    case Distance:
        result.value = calculateDistance();
        result.label = QString("%1 m").arg(result.value, 0, 'f', 2);
        break;
    case Area:
        result.value = calculateArea();
        result.label = QString("%1 m²").arg(result.value, 0, 'f', 2);
        break;
    case Volume:
        result.value = calculateVolume();
        result.label = QString("%1 m³").arg(result.value, 0, 'f', 2);
        break;
    case Angle:
        result.value = calculateAngle();
        result.label = QString("%1°").arg(result.value, 0, 'f', 1);
        break;
    }

    return result;
}

double MeasurementTool::calculateDistance()
{
    if (m_points.size() < 2) {
        return 0.0;
    }

    double totalDistance = 0.0;
    for (int i = 1; i < m_points.size(); ++i) {
        totalDistance += (m_points[i] - m_points[i - 1]).length();
    }

    return totalDistance;
}

double MeasurementTool::calculateArea()
{
    if (m_points.size() < 3) {
        return 0.0;
    }

    // Simplified planar area calculation
    double area = 0.0;
    for (int i = 1; i < m_points.size() - 1; ++i) {
        QVector3D v1 = m_points[i] - m_points[0];
        QVector3D v2 = m_points[i + 1] - m_points[0];
        area += QVector3D::crossProduct(v1, v2).length() * 0.5;
    }

    return area;
}

double MeasurementTool::calculateVolume()
{
    // Simplified - would use convex hull or alpha shapes in production
    return 0.0;
}

double MeasurementTool::calculateAngle()
{
    if (m_points.size() < 3) {
        return 0.0;
    }

    QVector3D v1 = (m_points[0] - m_points[1]).normalized();
    QVector3D v2 = (m_points[2] - m_points[1]).normalized();

    float dotProduct = QVector3D::dotProduct(v1, v2);
    return qRadiansToDegrees(acos(std::clamp(dotProduct, -1.0f, 1.0f)));
}

// PointCloudViewer implementation

PointCloudViewer::PointCloudViewer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_colorScheme(PointCloudColorMap::RGB)
    , m_measurementMode(false)
    , m_measurementType(MeasurementTool::Distance)
    , m_leftButtonPressed(false)
    , m_middleButtonPressed(false)
    , m_rightButtonPressed(false)
{
    setFocusPolicy(Qt::StrongFocus);
}

PointCloudViewer::~PointCloudViewer()
{
    makeCurrent();
    // Cleanup OpenGL resources
    doneCurrent();
}

bool PointCloudViewer::loadPointCloud(const QString& filePath)
{
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        emit renderingError("Point cloud file not found: " + filePath);
        return false;
    }

    QString extension = fileInfo.suffix().toLower();

    bool success = false;
    if (extension == "ply") {
        success = m_cloud.loadFromPLY(filePath);
    } else if (extension == "las" || extension == "laz") {
        success = m_cloud.loadFromLAS(filePath);
    } else if (extension == "xyz" || extension == "txt") {
        success = m_cloud.loadFromXYZ(filePath);
    } else {
        emit renderingError("Unsupported file format: " + extension);
        return false;
    }

    if (success) {
        // Build octree for efficient rendering
        m_octree.build(m_cloud, 100);

        // Center camera on point cloud
        m_camera.setTarget(m_cloud.centroid);

        update();
        emit pointCloudLoaded(filePath, m_cloud.size());
    }

    return success;
}

void PointCloudViewer::setPointCloud(const PointCloud& cloud)
{
    m_cloud = cloud;
    m_octree.build(m_cloud, 100);
    m_camera.setTarget(m_cloud.centroid);
    update();
}

void PointCloudViewer::setSettings(const PointCloudSettings& settings)
{
    m_settings = settings;
    update();
}

void PointCloudViewer::setColorScheme(PointCloudColorMap::Scheme scheme)
{
    m_colorScheme = scheme;
    update();
}

bool PointCloudViewer::exportPointCloud(const QString& filePath)
{
    return m_cloud.saveToPLY(filePath);
}

bool PointCloudViewer::exportImage(const QString& filePath)
{
    QImage image = grabFramebuffer();
    return image.save(filePath);
}

int PointCloudViewer::getPointAt(const QPoint& screenPos)
{
    return pickPoint(screenPos);
}

void PointCloudViewer::enableMeasurement(bool enabled, MeasurementTool::MeasurementType type)
{
    m_measurementMode = enabled;
    m_measurementType = type;

    if (!enabled) {
        m_measurement.clear();
    }
}

QString PointCloudViewer::getStatistics() const
{
    QString stats;
    stats += QString("Points: %1\n").arg(m_cloud.size());
    stats += QString("Bounds: [%1, %2, %3] to [%4, %5, %6]\n")
        .arg(m_cloud.minBounds.x(), 0, 'f', 2)
        .arg(m_cloud.minBounds.y(), 0, 'f', 2)
        .arg(m_cloud.minBounds.z(), 0, 'f', 2)
        .arg(m_cloud.maxBounds.x(), 0, 'f', 2)
        .arg(m_cloud.maxBounds.y(), 0, 'f', 2)
        .arg(m_cloud.maxBounds.z(), 0, 'f', 2);
    stats += QString("Centroid: [%1, %2, %3]\n")
        .arg(m_cloud.centroid.x(), 0, 'f', 2)
        .arg(m_cloud.centroid.y(), 0, 'f', 2)
        .arg(m_cloud.centroid.z(), 0, 'f', 2);
    stats += QString("Has colors: %1\n").arg(m_cloud.hasColors ? "Yes" : "No");
    stats += QString("Has normals: %1\n").arg(m_cloud.hasNormals ? "Yes" : "No");

    return stats;
}

void PointCloudViewer::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Generate test point cloud if none loaded
    if (m_cloud.isEmpty()) {
        m_cloud.generateTestCloud(10000);
        m_octree.build(m_cloud, 100);
        m_camera.setTarget(m_cloud.centroid);
    }
}

void PointCloudViewer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void PointCloudViewer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_cloud.isEmpty()) {
        return;
    }

    // Setup matrices (would use shaders in production)
    float aspect = static_cast<float>(width()) / height();
    QMatrix4x4 projection = m_camera.projectionMatrix(aspect);
    QMatrix4x4 view = m_camera.viewMatrix();

    // Render points
    if (m_settings.showPoints) {
        renderPoints();
    }

    // Render normals
    if (m_settings.showNormals && m_cloud.hasNormals) {
        renderNormals();
    }

    // Render measurements
    if (m_measurementMode) {
        renderMeasurements();
    }
}

void PointCloudViewer::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton) {
        if (m_measurementMode) {
            int pointIdx = pickPoint(event->pos());
            if (pointIdx >= 0) {
                m_measurement.addPoint(m_cloud.points[pointIdx].position);
                emit pointSelected(pointIdx);
            }
        } else {
            m_leftButtonPressed = true;
        }
    } else if (event->button() == Qt::MiddleButton) {
        m_middleButtonPressed = true;
    } else if (event->button() == Qt::RightButton) {
        m_rightButtonPressed = true;
    }
}

void PointCloudViewer::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_lastMousePos;

    if (m_leftButtonPressed) {
        m_camera.orbit(delta.x() * 0.5f, -delta.y() * 0.5f);
        update();
    } else if (m_middleButtonPressed || m_rightButtonPressed) {
        m_camera.pan(-delta.x() * 2.0f, delta.y() * 2.0f);
        update();
    }

    m_lastMousePos = event->pos();
}

void PointCloudViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftButtonPressed = false;
    } else if (event->button() == Qt::MiddleButton) {
        m_middleButtonPressed = false;
    } else if (event->button() == Qt::RightButton) {
        m_rightButtonPressed = false;
    }
}

void PointCloudViewer::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() * 0.5f;
    m_camera.zoom(delta);
    update();
}

void PointCloudViewer::renderPoints()
{
    glPointSize(m_settings.pointSize);

    glBegin(GL_POINTS);

    // Get visible points (simplified - would use octree in production)
    for (const auto& point : m_cloud.points) {
        QColor color = getPointColor(point);

        glColor4f(
            color.redF(),
            color.greenF(),
            color.blueF(),
            m_settings.opacity);

        glVertex3f(point.position.x(), point.position.y(), point.position.z());
    }

    glEnd();
}

void PointCloudViewer::renderNormals()
{
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);

    for (const auto& point : m_cloud.points) {
        QVector3D end = point.position + point.normal * m_settings.normalLength;

        glVertex3f(point.position.x(), point.position.y(), point.position.z());
        glVertex3f(end.x(), end.y(), end.z());
    }

    glEnd();
}

void PointCloudViewer::renderMeasurements()
{
    // Render measurement points and lines
    glPointSize(8.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_POINTS);

    // Draw measurement points (would render in screen space in production)

    glEnd();
}

void PointCloudViewer::renderBoundingBox()
{
    // Render bounding box
}

QColor PointCloudViewer::getPointColor(const Point& point)
{
    if (m_settings.usePointColor && m_cloud.hasColors) {
        return point.color;
    }

    if (m_settings.useColorMap) {
        switch (m_colorScheme) {
        case PointCloudColorMap::Height:
            return PointCloudColorMap::getColorForHeight(
                point.position.z(),
                m_cloud.minBounds.z(),
                m_cloud.maxBounds.z());
        case PointCloudColorMap::Intensity:
            return PointCloudColorMap::getColorForIntensity(point.intensity);
        case PointCloudColorMap::Classification:
            return PointCloudColorMap::getColorForClassification(point.classification);
        case PointCloudColorMap::Normal:
            return PointCloudColorMap::getColorForNormal(point.normal);
        case PointCloudColorMap::RGB:
            return point.color;
        default:
            return m_settings.defaultColor;
        }
    }

    return m_settings.defaultColor;
}

int PointCloudViewer::pickPoint(const QPoint& screenPos)
{
    Q_UNUSED(screenPos);
    // Would implement GPU-based picking in production
    return -1;
}

QVector3D PointCloudViewer::screenToWorld(const QPoint& screenPos, float depth)
{
    Q_UNUSED(screenPos);
    Q_UNUSED(depth);
    return QVector3D(0, 0, 0);
}

QVector<int> PointCloudViewer::getVisiblePoints()
{
    // Would use octree frustum culling in production
    QVector<int> indices;
    for (int i = 0; i < m_cloud.size(); ++i) {
        indices.append(i);
    }
    return indices;
}

} // namespace UI
} // namespace DroneMapper
