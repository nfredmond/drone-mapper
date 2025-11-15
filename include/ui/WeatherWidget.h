#ifndef WEATHERWIDGET_H
#define WEATHERWIDGET_H

#include "WeatherService.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QDateTime>

namespace DroneMapper {
namespace UI {

/**
 * @brief Widget displaying weather conditions and flight suitability
 *
 * Features:
 * - Current temperature, wind, humidity, pressure
 * - Wind direction indicator
 * - Flight suitability score with visual indicator
 * - Safety warnings
 * - Refresh button with auto-update timer
 * - Sunrise/sunset times
 * - Next 3-hour forecast
 */
class WeatherWidget : public QWidget {
    Q_OBJECT

public:
    explicit WeatherWidget(QWidget *parent = nullptr);
    ~WeatherWidget();

    void setLocation(double latitude, double longitude);
    void refreshWeather();

signals:
    void weatherUpdated(const Core::WeatherData& data);
    void locationChanged(double latitude, double longitude);

private slots:
    void onWeatherDataReceived(const Core::WeatherData& data);
    void onWeatherError(const QString& error);
    void onRefreshClicked();
    void onAutoRefreshTimeout();

private:
    void setupUI();
    void updateDisplay(const Core::WeatherData& data);
    QString getWindDirectionText(double degrees) const;
    QColor getSuitabilityColor(int score) const;
    QString formatTime(const QDateTime& dt) const;

    // UI Components
    QLabel *m_temperatureLabel;
    QLabel *m_feelsLikeLabel;
    QLabel *m_windLabel;
    QLabel *m_windDirectionLabel;
    QLabel *m_humidityLabel;
    QLabel *m_pressureLabel;
    QLabel *m_visibilityLabel;
    QLabel *m_cloudCoverLabel;
    QLabel *m_descriptionLabel;
    QLabel *m_sunTimesLabel;
    QLabel *m_forecastLabel;
    QLabel *m_warningsLabel;

    QProgressBar *m_suitabilityBar;
    QLabel *m_suitabilityLabel;

    QPushButton *m_refreshButton;
    QLabel *m_lastUpdateLabel;

    // Data
    double m_latitude;
    double m_longitude;
    Core::WeatherData m_currentData;

    // Auto-refresh timer
    QTimer *m_autoRefreshTimer;
};

} // namespace UI
} // namespace DroneMapper

#endif // WEATHERWIDGET_H
