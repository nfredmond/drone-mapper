#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QVariant>
#include <QSettings>
#include <memory>

namespace DroneMapper {
namespace Core {

/**
 * @brief Application settings manager
 */
class Settings {
public:
    static Settings& instance();

    // General settings
    QString defaultProjectPath() const;
    void setDefaultProjectPath(const QString& path);

    QString mapTileProvider() const;
    void setMapTileProvider(const QString& provider);

    // Flight planning defaults
    double defaultFlightAltitude() const;
    void setDefaultFlightAltitude(double altitude);

    double defaultFlightSpeed() const;
    void setDefaultFlightSpeed(double speed);

    double defaultFrontOverlap() const;
    void setDefaultFrontOverlap(double overlap);

    double defaultSideOverlap() const;
    void setDefaultSideOverlap(double overlap);

    // Processing settings
    bool useGPUAcceleration() const;
    void setUseGPUAcceleration(bool use);

    int processingQuality() const; // 0=low, 1=medium, 2=high
    void setProcessingQuality(int quality);

    // UI settings
    QString uiTheme() const;
    void setUITheme(const QString& theme);

    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray& geometry);

    QByteArray mainWindowState() const;
    void setMainWindowState(const QByteArray& state);

    // Generic get/set
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

    void sync();

private:
    Settings();
    ~Settings();
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    std::unique_ptr<QSettings> m_settings;
};

} // namespace Core
} // namespace DroneMapper

#endif // SETTINGS_H
