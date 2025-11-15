#ifndef WINDOVERLAYWIDGET_H
#define WINDOVERLAYWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include "models/GeospatialCoordinate.h"
#include "core/WeatherService.h"

namespace DroneMapper {
namespace UI {

/**
 * @brief Wind data point for visualization
 */
struct WindDataPoint {
    Models::GeospatialCoordinate location;
    double windSpeed;           // m/s
    double windDirection;       // degrees (0 = North, 90 = East)
    double gustSpeed;          // m/s
    QDateTime timestamp;
    QString source;            // "METAR", "GRIB", "Forecast", etc.

    // Helper methods
    double getBeaufortScale() const;
    QString getCardinalDirection() const;
    QString getSpeedKnots() const;
    QString getSpeedKmh() const;
};

/**
 * @brief Wind visualization settings
 */
struct WindOverlaySettings {
    bool showArrows;
    bool showBarbs;            // Meteorological wind barbs
    bool showStreamlines;      // Flow visualization
    bool showText;             // Speed/direction labels

    double arrowScale;         // Arrow size multiplier
    double arrowSpacing;       // Grid spacing in meters
    double minWindSpeed;       // Don't show below this speed

    bool colorBySpeed;
    bool animateFlow;
    double animationSpeed;

    int opacity;               // 0-100

    WindOverlaySettings();
};

/**
 * @brief Color scheme for wind speed visualization
 */
class WindColorScheme {
public:
    static QColor getColorForSpeed(double speedMs, double maxSpeed = 25.0);
    static QColor getColorBeaufort(int beaufortScale);

    // Predefined schemes
    enum Scheme {
        Rainbow,        // Blue → Green → Yellow → Red
        Thermal,        // Blue → Red
        Monochrome,     // White with varying opacity
        Aviation        // Standard aviation colors
    };

    static QColor getColor(double speedMs, Scheme scheme = Rainbow);
};

/**
 * @brief Wind overlay widget for map visualization
 *
 * Features:
 * - Real-time wind direction arrows
 * - Meteorological wind barbs (standard aviation)
 * - Wind speed color coding
 * - Streamline visualization
 * - METAR/TAF data integration
 * - GRIB forecast data support
 * - Animation for dynamic visualization
 * - Configurable display options
 *
 * Usage:
 *   WindOverlayWidget* overlay = new WindOverlayWidget(parent);
 *   overlay->setWeatherService(weatherService);
 *   overlay->setViewBounds(topLeft, bottomRight);
 *   overlay->refreshData();
 */
class WindOverlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit WindOverlayWidget(QWidget* parent = nullptr);
    ~WindOverlayWidget() override;

    /**
     * @brief Set weather service for data
     * @param service Weather service instance
     */
    void setWeatherService(Core::WeatherService* service);

    /**
     * @brief Set map view bounds
     * @param topLeft Top-left coordinate
     * @param bottomRight Bottom-right coordinate
     */
    void setViewBounds(
        const Models::GeospatialCoordinate& topLeft,
        const Models::GeospatialCoordinate& bottomRight);

    /**
     * @brief Set display settings
     * @param settings Overlay settings
     */
    void setSettings(const WindOverlaySettings& settings);

    /**
     * @brief Get current settings
     * @return Current settings
     */
    WindOverlaySettings settings() const { return m_settings; }

    /**
     * @brief Add wind data point
     * @param point Wind data point
     */
    void addWindDataPoint(const WindDataPoint& point);

    /**
     * @brief Clear all wind data
     */
    void clearData();

    /**
     * @brief Refresh wind data from service
     */
    void refreshData();

    /**
     * @brief Set coordinate to screen pixel transform
     * @param func Transform function
     */
    void setCoordinateTransform(
        std::function<QPointF(const Models::GeospatialCoordinate&)> func);

    /**
     * @brief Enable/disable overlay
     * @param enabled Enable state
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if overlay is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }

signals:
    void dataRefreshed();
    void errorOccurred(const QString& error);

protected:
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;

private:
    Core::WeatherService* m_weatherService;
    QList<WindDataPoint> m_windData;
    WindOverlaySettings m_settings;

    Models::GeospatialCoordinate m_topLeft;
    Models::GeospatialCoordinate m_bottomRight;

    std::function<QPointF(const Models::GeospatialCoordinate&)> m_coordTransform;

    bool m_enabled;
    int m_animationTimerId;
    double m_animationPhase;

    // Rendering methods
    void drawWindArrow(
        QPainter& painter,
        const QPointF& position,
        double windSpeed,
        double windDirection);

    void drawWindBarb(
        QPainter& painter,
        const QPointF& position,
        double windSpeed,
        double windDirection);

    void drawWindText(
        QPainter& painter,
        const QPointF& position,
        const WindDataPoint& data);

    void drawStreamlines(QPainter& painter);

    // Helper methods
    QList<WindDataPoint> generateGridPoints();
    QList<WindDataPoint> interpolateWindField();
    WindDataPoint interpolateWindAt(const Models::GeospatialCoordinate& location);

    QPointF coordinateToScreen(const Models::GeospatialCoordinate& coord) const;

    // Arrow geometry
    QPolygonF createArrowPolygon(double length, double width) const;
    void drawFeather(QPainter& painter, double offset, bool full);
};

/**
 * @brief Wind barb renderer (standard meteorological)
 *
 * Wind barbs follow WMO standard:
 * - Short barb = 5 knots
 * - Long barb = 10 knots
 * - Pennant = 50 knots
 */
class WindBarbRenderer {
public:
    /**
     * @brief Draw wind barb
     * @param painter QPainter instance
     * @param center Center point
     * @param speedKnots Wind speed in knots
     * @param direction Direction in degrees
     * @param scale Scale factor
     */
    static void drawBarb(
        QPainter& painter,
        const QPointF& center,
        double speedKnots,
        double direction,
        double scale = 1.0);

private:
    static void addShortBarb(QPainter& painter, const QPointF& point, double angle);
    static void addLongBarb(QPainter& painter, const QPointF& point, double angle);
    static void addPennant(QPainter& painter, const QPointF& point, double angle);
};

} // namespace UI
} // namespace DroneMapper

#endif // WINDOVERLAYWIDGET_H
