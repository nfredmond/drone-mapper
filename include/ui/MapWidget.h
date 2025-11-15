#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "MapBridge.h"
#include "FlightPlan.h"
#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>

namespace DroneMapper {
namespace UI {

/**
 * @brief Interactive map widget using MapLibre GL via Qt WebEngine
 */
class MapWidget : public QWidget {
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr);
    ~MapWidget();

    /**
     * @brief Set map center coordinates
     */
    void setCenter(double latitude, double longitude, int zoom = 14);

    /**
     * @brief Display flight path on map
     */
    void displayFlightPath(const Models::FlightPlan& plan);

    /**
     * @brief Update flight information panel
     */
    void updateFlightInfo(double distance, int timeSeconds, int photoCount);

    /**
     * @brief Clear all drawings from map
     */
    void clearMap();

    /**
     * @brief Switch base map layer
     */
    void setBaseMap(const QString& type); // "satellite" or "street"

    /**
     * @brief Get the map bridge for signal connections
     */
    MapBridge* bridge() { return m_bridge; }

signals:
    void areaSelected(const QString& geojson);
    void flightPlanRequested();
    void mapClicked(double lat, double lng);

private slots:
    void onMapReady();
    void onAreaDrawn(const QString& geojson);
    void onFlightPlanRequested(const QString& geojson);

private:
    void setupWebChannel();
    QString loadMapHtml();
    QString flightPlanToGeoJson(const Models::FlightPlan& plan);

    QWebEngineView* m_webView;
    QWebChannel* m_channel;
    MapBridge* m_bridge;

    QString m_currentAreaGeoJson;
};

} // namespace UI
} // namespace DroneMapper

#endif // MAPWIDGET_H
