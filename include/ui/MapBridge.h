#ifndef MAPBRIDGE_H
#define MAPBRIDGE_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>

namespace DroneMapper {
namespace UI {

/**
 * @brief Bridge between Qt C++ and JavaScript map interface
 *
 * Uses Qt WebChannel to communicate with MapLibre GL running in WebEngine
 */
class MapBridge : public QObject {
    Q_OBJECT

public:
    explicit MapBridge(QObject *parent = nullptr);

signals:
    // Signals emitted to notify Qt application
    void mapReady();
    void areaDrawn(const QString& geojson);
    void flightPlanRequested(const QString& geojson);
    void waypointAdded(double latitude, double longitude);
    void mapClicked(double latitude, double longitude);

public slots:
    // Slots called from JavaScript
    void onMapReady();
    void onAreaDrawn(const QString& geojson);
    void onGenerateFlightPlan(const QString& geojson);
    void onWaypointClick(double lat, double lng);
    void onMapClick(double lat, double lng);
};

} // namespace UI
} // namespace DroneMapper

#endif // MAPBRIDGE_H
