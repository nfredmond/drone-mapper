#include "Settings.h"
#include <QStandardPaths>
#include <QCoreApplication>

namespace DroneMapper {
namespace Core {

Settings& Settings::instance()
{
    static Settings instance;
    return instance;
}

Settings::Settings()
{
    QCoreApplication::setOrganizationName("DroneMapper");
    QCoreApplication::setApplicationName("DroneMapper");

    m_settings = std::make_unique<QSettings>(
        QSettings::IniFormat,
        QSettings::UserScope,
        "DroneMapper",
        "DroneMapper"
    );
}

Settings::~Settings()
{
    sync();
}

QString Settings::defaultProjectPath() const
{
    return value("General/DefaultProjectPath",
                 QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/DroneMapper")
           .toString();
}

void Settings::setDefaultProjectPath(const QString& path)
{
    setValue("General/DefaultProjectPath", path);
}

QString Settings::mapTileProvider() const
{
    return value("Map/TileProvider", "OpenStreetMap").toString();
}

void Settings::setMapTileProvider(const QString& provider)
{
    setValue("Map/TileProvider", provider);
}

double Settings::defaultFlightAltitude() const
{
    return value("FlightPlanning/DefaultAltitude", 75.0).toDouble();
}

void Settings::setDefaultFlightAltitude(double altitude)
{
    setValue("FlightPlanning/DefaultAltitude", altitude);
}

double Settings::defaultFlightSpeed() const
{
    return value("FlightPlanning/DefaultSpeed", 8.0).toDouble();
}

void Settings::setDefaultFlightSpeed(double speed)
{
    setValue("FlightPlanning/DefaultSpeed", speed);
}

double Settings::defaultFrontOverlap() const
{
    return value("FlightPlanning/DefaultFrontOverlap", 75.0).toDouble();
}

void Settings::setDefaultFrontOverlap(double overlap)
{
    setValue("FlightPlanning/DefaultFrontOverlap", overlap);
}

double Settings::defaultSideOverlap() const
{
    return value("FlightPlanning/DefaultSideOverlap", 65.0).toDouble();
}

void Settings::setDefaultSideOverlap(double overlap)
{
    setValue("FlightPlanning/DefaultSideOverlap", overlap);
}

bool Settings::useGPUAcceleration() const
{
    return value("Processing/UseGPU", true).toBool();
}

void Settings::setUseGPUAcceleration(bool use)
{
    setValue("Processing/UseGPU", use);
}

int Settings::processingQuality() const
{
    return value("Processing/Quality", 1).toInt();
}

void Settings::setProcessingQuality(int quality)
{
    setValue("Processing/Quality", quality);
}

QString Settings::uiTheme() const
{
    return value("UI/Theme", "Light").toString();
}

void Settings::setUITheme(const QString& theme)
{
    setValue("UI/Theme", theme);
}

QByteArray Settings::mainWindowGeometry() const
{
    return value("UI/MainWindowGeometry").toByteArray();
}

void Settings::setMainWindowGeometry(const QByteArray& geometry)
{
    setValue("UI/MainWindowGeometry", geometry);
}

QByteArray Settings::mainWindowState() const
{
    return value("UI/MainWindowState").toByteArray();
}

void Settings::setMainWindowState(const QByteArray& state)
{
    setValue("UI/MainWindowState", state);
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void Settings::setValue(const QString& key, const QVariant& value)
{
    m_settings->setValue(key, value);
}

void Settings::sync()
{
    m_settings->sync();
}

} // namespace Core
} // namespace DroneMapper
