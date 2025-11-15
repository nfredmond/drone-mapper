#include "MapWidget.h"
#include "Logger.h"
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

namespace DroneMapper {
namespace UI {

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent)
    , m_webView(new QWebEngineView(this))
    , m_channel(new QWebChannel(this))
    , m_bridge(new MapBridge(this))
{
    // Setup layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    // Enable web features
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    // Setup WebChannel
    setupWebChannel();

    // Connect bridge signals
    connect(m_bridge, &MapBridge::mapReady, this, &MapWidget::onMapReady);
    connect(m_bridge, &MapBridge::areaDrawn, this, &MapWidget::onAreaDrawn);
    connect(m_bridge, &MapBridge::flightPlanRequested, this, &MapWidget::onFlightPlanRequested);

    // Load map
    QString html = loadMapHtml();
    m_webView->setHtml(html, QUrl("qrc:/"));

    LOG_INFO("MapWidget initialized");
}

MapWidget::~MapWidget()
{
}

void MapWidget::setupWebChannel()
{
    m_channel->registerObject("mapBridge", m_bridge);
    m_webView->page()->setWebChannel(m_channel);
}

QString MapWidget::loadMapHtml()
{
    // Try to load from resources or file
    QString mapPath = ":/map/map.html";
    QFile file(mapPath);

    if (!file.exists()) {
        // Try local file path
        QString localPath = QDir::currentPath() + "/resources/map/map.html";
        QFile localFile(localPath);

        if (localFile.open(QIODevice::ReadOnly)) {
            QString html = QString::fromUtf8(localFile.readAll());
            localFile.close();
            return html;
        } else {
            LOG_ERROR("Could not load map.html from " + localPath);
        }
    } else {
        if (file.open(QIODevice::ReadOnly)) {
            QString html = QString::fromUtf8(file.readAll());
            file.close();
            return html;
        }
    }

    // Fallback: inline simple map
    return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>DroneMapper Map</title>
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <style>
        body { margin: 0; padding: 20px; font-family: Arial; }
        .placeholder { padding: 40px; text-align: center; background: #f0f0f0; border-radius: 8px; }
    </style>
</head>
<body>
    <div class="placeholder">
        <h2>Map Loading...</h2>
        <p>Map interface will appear here</p>
        <p>Location: resources/map/map.html</p>
    </div>
    <script>
        new QWebChannel(qt.webChannelTransport, function(channel) {
            var bridge = channel.objects.mapBridge;
            if (bridge && bridge.onMapReady) {
                bridge.onMapReady();
            }
        });
    </script>
</body>
</html>
    )";
}

void MapWidget::setCenter(double latitude, double longitude, int zoom)
{
    QString js = QString("if (typeof setCenter === 'function') { setCenter(%1, %2, %3); }")
                .arg(longitude)
                .arg(latitude)
                .arg(zoom);
    m_webView->page()->runJavaScript(js);
}

void MapWidget::displayFlightPath(const Models::FlightPlan& plan)
{
    QString geojson = flightPlanToGeoJson(plan);
    QString js = QString("if (typeof displayFlightPath === 'function') { displayFlightPath('%1'); }")
                .arg(geojson.replace("'", "\\'"));
    m_webView->page()->runJavaScript(js);
}

void MapWidget::updateFlightInfo(double distance, int timeSeconds, int photoCount)
{
    QJsonObject info;
    info["distance"] = distance;
    info["time"] = timeSeconds;
    info["photos"] = photoCount;

    QString json = QString::fromUtf8(QJsonDocument(info).toJson(QJsonDocument::Compact));
    QString js = QString("if (typeof updateFlightInfo === 'function') { updateFlightInfo('%1'); }")
                .arg(json.replace("'", "\\'"));
    m_webView->page()->runJavaScript(js);
}

void MapWidget::clearMap()
{
    QString js = "if (typeof clearFlightPath === 'function') { clearFlightPath(); }";
    m_webView->page()->runJavaScript(js);
}

void MapWidget::setBaseMap(const QString& type)
{
    QString js = QString("if (typeof switchBaseMap === 'function') { switchBaseMap('%1'); }")
                .arg(type);
    m_webView->page()->runJavaScript(js);
}

QString MapWidget::flightPlanToGeoJson(const Models::FlightPlan& plan)
{
    QJsonObject geojson;
    geojson["type"] = "FeatureCollection";

    QJsonArray features;

    // Add waypoints as Point features
    const auto& waypoints = plan.waypoints();
    for (int i = 0; i < waypoints.count(); ++i) {
        const auto& wp = waypoints[i];
        const auto& coord = wp.coordinate();

        QJsonObject feature;
        feature["type"] = "Feature";

        QJsonObject geometry;
        geometry["type"] = "Point";
        QJsonArray coordinates;
        coordinates.append(coord.longitude());
        coordinates.append(coord.latitude());
        geometry["coordinates"] = coordinates;
        feature["geometry"] = geometry;

        QJsonObject properties;
        properties["number"] = i + 1;
        properties["altitude"] = coord.altitude();
        feature["properties"] = properties;

        features.append(feature);
    }

    // Add flight path as LineString
    if (waypoints.count() > 1) {
        QJsonObject lineFeature;
        lineFeature["type"] = "Feature";

        QJsonObject geometry;
        geometry["type"] = "LineString";

        QJsonArray coordinates;
        for (const auto& wp : waypoints) {
            const auto& coord = wp.coordinate();
            QJsonArray point;
            point.append(coord.longitude());
            point.append(coord.latitude());
            coordinates.append(point);
        }
        geometry["coordinates"] = coordinates;
        lineFeature["geometry"] = geometry;

        QJsonObject properties;
        properties["type"] = "flight-path";
        lineFeature["properties"] = properties;

        features.append(lineFeature);
    }

    geojson["features"] = features;

    return QString::fromUtf8(QJsonDocument(geojson).toJson(QJsonDocument::Compact));
}

void MapWidget::onMapReady()
{
    LOG_INFO("Map ready signal received");
}

void MapWidget::onAreaDrawn(const QString& geojson)
{
    m_currentAreaGeoJson = geojson;
    emit areaSelected(geojson);
}

void MapWidget::onFlightPlanRequested(const QString& geojson)
{
    m_currentAreaGeoJson = geojson;
    emit flightPlanRequested();
}

} // namespace UI
} // namespace DroneMapper
