#include "WindOverlayWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTimerEvent>
#include <QtMath>
#include <cmath>

namespace DroneMapper {
namespace UI {

// WindDataPoint implementation

double WindDataPoint::getBeaufortScale() const
{
    // Beaufort scale conversion from m/s
    if (windSpeed < 0.5) return 0;      // Calm
    if (windSpeed < 1.6) return 1;      // Light air
    if (windSpeed < 3.4) return 2;      // Light breeze
    if (windSpeed < 5.5) return 3;      // Gentle breeze
    if (windSpeed < 8.0) return 4;      // Moderate breeze
    if (windSpeed < 10.8) return 5;     // Fresh breeze
    if (windSpeed < 13.9) return 6;     // Strong breeze
    if (windSpeed < 17.2) return 7;     // High wind
    if (windSpeed < 20.8) return 8;     // Gale
    if (windSpeed < 24.5) return 9;     // Strong gale
    if (windSpeed < 28.5) return 10;    // Storm
    if (windSpeed < 32.7) return 11;    // Violent storm
    return 12;                          // Hurricane
}

QString WindDataPoint::getCardinalDirection() const
{
    // Convert degrees to cardinal direction
    double normalized = fmod(windDirection + 360.0, 360.0);

    if (normalized >= 348.75 || normalized < 11.25) return "N";
    if (normalized < 33.75) return "NNE";
    if (normalized < 56.25) return "NE";
    if (normalized < 78.75) return "ENE";
    if (normalized < 101.25) return "E";
    if (normalized < 123.75) return "ESE";
    if (normalized < 146.25) return "SE";
    if (normalized < 168.75) return "SSE";
    if (normalized < 191.25) return "S";
    if (normalized < 213.75) return "SSW";
    if (normalized < 236.25) return "SW";
    if (normalized < 258.75) return "WSW";
    if (normalized < 281.25) return "W";
    if (normalized < 303.75) return "WNW";
    if (normalized < 326.25) return "NW";
    if (normalized < 348.75) return "NNW";

    return "N";
}

QString WindDataPoint::getSpeedKnots() const
{
    return QString::number(windSpeed * 1.94384, 'f', 1) + " kt";
}

QString WindDataPoint::getSpeedKmh() const
{
    return QString::number(windSpeed * 3.6, 'f', 1) + " km/h";
}

// WindOverlaySettings implementation

WindOverlaySettings::WindOverlaySettings()
    : showArrows(true)
    , showBarbs(false)
    , showStreamlines(false)
    , showText(true)
    , arrowScale(1.0)
    , arrowSpacing(1000.0)  // 1km grid
    , minWindSpeed(0.5)     // 0.5 m/s minimum
    , colorBySpeed(true)
    , animateFlow(true)
    , animationSpeed(1.0)
    , opacity(80)
{
}

// WindColorScheme implementation

QColor WindColorScheme::getColorForSpeed(double speedMs, double maxSpeed)
{
    // Normalize speed to 0-1 range
    double normalized = std::min(speedMs / maxSpeed, 1.0);

    if (normalized < 0.2) {
        // Blue (calm)
        return QColor(100, 150, 255);
    } else if (normalized < 0.4) {
        // Green
        return QColor(100, 255, 100);
    } else if (normalized < 0.6) {
        // Yellow
        return QColor(255, 255, 100);
    } else if (normalized < 0.8) {
        // Orange
        return QColor(255, 150, 50);
    } else {
        // Red (strong)
        return QColor(255, 50, 50);
    }
}

QColor WindColorScheme::getColorBeaufort(int beaufortScale)
{
    switch (beaufortScale) {
    case 0:  return QColor(200, 200, 255);  // Calm - light blue
    case 1:
    case 2:  return QColor(100, 200, 255);  // Light breeze - blue
    case 3:
    case 4:  return QColor(100, 255, 150);  // Moderate - green
    case 5:
    case 6:  return QColor(255, 255, 100);  // Fresh - yellow
    case 7:
    case 8:  return QColor(255, 180, 50);   // Strong - orange
    case 9:
    case 10: return QColor(255, 100, 50);   // Gale - red
    default: return QColor(200, 50, 50);    // Storm - dark red
    }
}

QColor WindColorScheme::getColor(double speedMs, Scheme scheme)
{
    switch (scheme) {
    case Rainbow:
        return getColorForSpeed(speedMs, 25.0);
    case Thermal: {
        double normalized = std::min(speedMs / 25.0, 1.0);
        int r = static_cast<int>(normalized * 255);
        int b = static_cast<int>((1.0 - normalized) * 255);
        return QColor(r, 0, b);
    }
    case Monochrome: {
        int alpha = static_cast<int>(std::min(speedMs / 25.0, 1.0) * 255);
        return QColor(255, 255, 255, alpha);
    }
    case Aviation:
        return getColorBeaufort(static_cast<int>(speedMs / 1.6));
    default:
        return QColor(255, 255, 255);
    }
}

// WindOverlayWidget implementation

WindOverlayWidget::WindOverlayWidget(QWidget* parent)
    : QWidget(parent)
    , m_weatherService(nullptr)
    , m_enabled(true)
    , m_animationTimerId(-1)
    , m_animationPhase(0.0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);

    // Default coordinate transform (identity)
    m_coordTransform = [](const Models::GeospatialCoordinate& coord) {
        return QPointF(coord.longitude(), coord.latitude());
    };

    // Start animation timer
    if (m_settings.animateFlow) {
        m_animationTimerId = startTimer(50); // 20 FPS
    }
}

WindOverlayWidget::~WindOverlayWidget()
{
    if (m_animationTimerId >= 0) {
        killTimer(m_animationTimerId);
    }
}

void WindOverlayWidget::setWeatherService(Core::WeatherService* service)
{
    m_weatherService = service;
}

void WindOverlayWidget::setViewBounds(
    const Models::GeospatialCoordinate& topLeft,
    const Models::GeospatialCoordinate& bottomRight)
{
    m_topLeft = topLeft;
    m_bottomRight = bottomRight;
    update();
}

void WindOverlayWidget::setSettings(const WindOverlaySettings& settings)
{
    m_settings = settings;

    // Update animation timer
    if (m_settings.animateFlow && m_animationTimerId < 0) {
        m_animationTimerId = startTimer(50);
    } else if (!m_settings.animateFlow && m_animationTimerId >= 0) {
        killTimer(m_animationTimerId);
        m_animationTimerId = -1;
    }

    update();
}

void WindOverlayWidget::addWindDataPoint(const WindDataPoint& point)
{
    m_windData.append(point);
    update();
}

void WindOverlayWidget::clearData()
{
    m_windData.clear();
    update();
}

void WindOverlayWidget::refreshData()
{
    if (!m_weatherService) {
        emit errorOccurred("No weather service configured");
        return;
    }

    // Generate grid points for wind data
    QList<WindDataPoint> gridPoints = generateGridPoints();

    // Request weather data for each grid point
    // (Simplified - in production would batch request)
    clearData();

    for (const auto& gridPoint : gridPoints) {
        // Query weather service
        // This is a simplified example - actual implementation would use
        // asynchronous weather data fetching
        WindDataPoint point = gridPoint;
        // point.windSpeed = ...
        // point.windDirection = ...
        addWindDataPoint(point);
    }

    emit dataRefreshed();
}

void WindOverlayWidget::setCoordinateTransform(
    std::function<QPointF(const Models::GeospatialCoordinate&)> func)
{
    m_coordTransform = func;
    update();
}

void WindOverlayWidget::setEnabled(bool enabled)
{
    m_enabled = enabled;
    update();
}

void WindOverlayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (!m_enabled || m_windData.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set opacity
    painter.setOpacity(m_settings.opacity / 100.0);

    // Draw streamlines first (background)
    if (m_settings.showStreamlines) {
        drawStreamlines(painter);
    }

    // Draw wind data points
    for (const auto& dataPoint : m_windData) {
        if (dataPoint.windSpeed < m_settings.minWindSpeed) {
            continue; // Skip calm winds
        }

        QPointF screenPos = coordinateToScreen(dataPoint.location);

        // Check if point is visible
        if (!rect().contains(screenPos.toPoint())) {
            continue;
        }

        // Set color based on speed
        QColor color;
        if (m_settings.colorBySpeed) {
            color = WindColorScheme::getColorForSpeed(dataPoint.windSpeed);
        } else {
            color = QColor(255, 255, 255);
        }

        painter.setPen(QPen(color, 2));
        painter.setBrush(color);

        // Draw arrows or barbs
        if (m_settings.showArrows) {
            drawWindArrow(painter, screenPos, dataPoint.windSpeed, dataPoint.windDirection);
        }

        if (m_settings.showBarbs) {
            drawWindBarb(painter, screenPos, dataPoint.windSpeed, dataPoint.windDirection);
        }

        // Draw text labels
        if (m_settings.showText) {
            drawWindText(painter, screenPos, dataPoint);
        }
    }
}

void WindOverlayWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_animationTimerId) {
        m_animationPhase += m_settings.animationSpeed * 0.05;
        if (m_animationPhase > 2.0 * M_PI) {
            m_animationPhase -= 2.0 * M_PI;
        }
        update();
    }
}

void WindOverlayWidget::drawWindArrow(
    QPainter& painter,
    const QPointF& position,
    double windSpeed,
    double windDirection)
{
    painter.save();

    // Move to position and rotate
    painter.translate(position);
    painter.rotate(windDirection);  // Direction wind is coming FROM

    // Arrow length based on wind speed
    double baseLength = 30.0 * m_settings.arrowScale;
    double length = baseLength + (windSpeed * 2.0);

    // Arrow head
    QPolygonF arrow = createArrowPolygon(length, 8.0);

    // Draw arrow
    painter.drawPolygon(arrow);

    // Add animation pulse
    if (m_settings.animateFlow) {
        double pulseScale = 1.0 + 0.2 * sin(m_animationPhase);
        painter.scale(pulseScale, pulseScale);
        painter.setOpacity(0.5);
        painter.drawPolygon(arrow);
    }

    painter.restore();
}

void WindOverlayWidget::drawWindBarb(
    QPainter& painter,
    const QPointF& position,
    double windSpeed,
    double windDirection)
{
    // Convert m/s to knots for standard wind barb
    double speedKnots = windSpeed * 1.94384;

    WindBarbRenderer::drawBarb(painter, position, speedKnots, windDirection, m_settings.arrowScale);
}

void WindOverlayWidget::drawWindText(
    QPainter& painter,
    const QPointF& position,
    const WindDataPoint& data)
{
    painter.save();

    // Draw background
    QString text = QString("%1 %2")
        .arg(data.getSpeedKnots())
        .arg(data.getCardinalDirection());

    QFontMetrics fm(painter.font());
    QRectF textRect = fm.boundingRect(text);
    textRect.moveCenter(position + QPointF(0, 25));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.drawRoundedRect(textRect.adjusted(-3, -2, 3, 2), 3, 3);

    // Draw text
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, text);

    painter.restore();
}

void WindOverlayWidget::drawStreamlines(QPainter& painter)
{
    // Simplified streamline visualization
    // In production, would use proper flow field integration

    painter.save();
    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));

    // This is a placeholder - actual streamlines require
    // Runge-Kutta integration through the wind field
    // For now, just draw flow lines based on local wind direction

    painter.restore();
}

QList<WindDataPoint> WindOverlayWidget::generateGridPoints()
{
    QList<WindDataPoint> points;

    // Calculate grid spacing in degrees
    double latSpacing = (m_settings.arrowSpacing / 111000.0); // ~111km per degree lat
    double lonSpacing = latSpacing; // Simplified

    // Generate grid
    double lat = m_topLeft.latitude();
    while (lat > m_bottomRight.latitude()) {
        double lon = m_topLeft.longitude();
        while (lon < m_bottomRight.longitude()) {
            WindDataPoint point;
            point.location = Models::GeospatialCoordinate(lat, lon, 0);
            point.windSpeed = 0;
            point.windDirection = 0;
            point.timestamp = QDateTime::currentDateTime();
            points.append(point);

            lon += lonSpacing;
        }
        lat -= latSpacing;
    }

    return points;
}

QList<WindDataPoint> WindOverlayWidget::interpolateWindField()
{
    // Bilinear interpolation of wind field
    // Simplified implementation
    return m_windData;
}

WindDataPoint WindOverlayWidget::interpolateWindAt(const Models::GeospatialCoordinate& location)
{
    Q_UNUSED(location);

    // Find nearest wind data points and interpolate
    // Simplified - return first point
    if (!m_windData.isEmpty()) {
        return m_windData.first();
    }

    return WindDataPoint();
}

QPointF WindOverlayWidget::coordinateToScreen(const Models::GeospatialCoordinate& coord) const
{
    if (m_coordTransform) {
        return m_coordTransform(coord);
    }

    // Fallback - simple linear mapping
    double x = (coord.longitude() - m_topLeft.longitude()) /
               (m_bottomRight.longitude() - m_topLeft.longitude()) * width();
    double y = (coord.latitude() - m_topLeft.latitude()) /
               (m_bottomRight.latitude() - m_topLeft.latitude()) * height();

    return QPointF(x, y);
}

QPolygonF WindOverlayWidget::createArrowPolygon(double length, double width) const
{
    QPolygonF arrow;

    // Arrow shaft
    arrow << QPointF(0, 0) << QPointF(-length, 0);

    // Arrow head
    double headLength = length * 0.3;
    double headWidth = width;

    arrow << QPointF(-length + headLength, headWidth / 2);
    arrow << QPointF(-length, 0);
    arrow << QPointF(-length + headLength, -headWidth / 2);
    arrow << QPointF(-length, 0);

    return arrow;
}

void WindOverlayWidget::drawFeather(QPainter& painter, double offset, bool full)
{
    Q_UNUSED(painter);
    Q_UNUSED(offset);
    Q_UNUSED(full);
    // Helper for wind barb feathers
}

// WindBarbRenderer implementation

void WindBarbRenderer::drawBarb(
    QPainter& painter,
    const QPointF& center,
    double speedKnots,
    double direction,
    double scale)
{
    painter.save();

    painter.translate(center);
    painter.rotate(direction);

    // Wind barb staff
    double staffLength = 40.0 * scale;
    painter.drawLine(QPointF(0, 0), QPointF(0, -staffLength));

    // Add barbs based on speed
    int roundedSpeed = static_cast<int>(speedKnots / 5) * 5; // Round to nearest 5 knots

    double barbOffset = -staffLength * 0.8;
    double barbSpacing = 7.0 * scale;

    // Pennants (50 knots each)
    while (roundedSpeed >= 50) {
        addPennant(painter, QPointF(0, barbOffset), direction);
        barbOffset += barbSpacing;
        roundedSpeed -= 50;
    }

    // Long barbs (10 knots each)
    while (roundedSpeed >= 10) {
        addLongBarb(painter, QPointF(0, barbOffset), direction);
        barbOffset += barbSpacing;
        roundedSpeed -= 10;
    }

    // Short barb (5 knots)
    if (roundedSpeed >= 5) {
        addShortBarb(painter, QPointF(0, barbOffset), direction);
    }

    painter.restore();
}

void WindBarbRenderer::addShortBarb(QPainter& painter, const QPointF& point, double angle)
{
    Q_UNUSED(angle);

    painter.save();
    painter.translate(point);

    // Short barb: 5 knots
    double length = 7.0;
    painter.drawLine(QPointF(0, 0), QPointF(length, -length));

    painter.restore();
}

void WindBarbRenderer::addLongBarb(QPainter& painter, const QPointF& point, double angle)
{
    Q_UNUSED(angle);

    painter.save();
    painter.translate(point);

    // Long barb: 10 knots
    double length = 14.0;
    painter.drawLine(QPointF(0, 0), QPointF(length, -length));

    painter.restore();
}

void WindBarbRenderer::addPennant(QPainter& painter, const QPointF& point, double angle)
{
    Q_UNUSED(angle);

    painter.save();
    painter.translate(point);

    // Pennant: 50 knots (filled triangle)
    QPolygonF pennant;
    pennant << QPointF(0, 0)
            << QPointF(14, -7)
            << QPointF(0, -14);

    painter.drawPolygon(pennant);

    painter.restore();
}

} // namespace UI
} // namespace DroneMapper
