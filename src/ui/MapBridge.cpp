#include "MapBridge.h"
#include "Logger.h"
#include <QDebug>

namespace DroneMapper {
namespace UI {

MapBridge::MapBridge(QObject *parent)
    : QObject(parent)
{
    LOG_INFO("MapBridge created");
}

void MapBridge::onMapReady()
{
    LOG_INFO("Map interface ready");
    emit mapReady();
}

void MapBridge::onAreaDrawn(const QString& geojson)
{
    LOG_DEBUG(QString("Area drawn: %1").arg(geojson.left(100)));
    emit areaDrawn(geojson);
}

void MapBridge::onGenerateFlightPlan(const QString& geojson)
{
    LOG_INFO("Flight plan generation requested");
    emit flightPlanRequested(geojson);
}

void MapBridge::onWaypointClick(double lat, double lng)
{
    LOG_DEBUG(QString("Waypoint clicked: %1, %2").arg(lat).arg(lng));
    emit waypointAdded(lat, lng);
}

void MapBridge::onMapClick(double lat, double lng)
{
    emit mapClicked(lat, lng);
}

} // namespace UI
} // namespace DroneMapper
