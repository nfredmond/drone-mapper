#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include "GeospatialCoordinate.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>

namespace DroneMapper {
namespace Core {

/**
 * @brief Weather data for flight planning
 */
struct WeatherData {
    // Current conditions
    double temperature;        // Celsius
    double feelsLike;         // Celsius
    double windSpeed;         // m/s
    double windDirection;     // degrees
    double windGust;          // m/s
    double humidity;          // percentage
    double pressure;          // hPa
    double visibility;        // meters
    double cloudCover;        // percentage
    double precipitation;     // mm
    QString description;      // e.g. "clear sky", "light rain"
    QString icon;            // weather icon code

    // Sun data
    QDateTime sunrise;
    QDateTime sunset;

    // Forecast data (next 3 hours)
    double tempForecast;
    double windForecast;
    QString forecastDesc;

    // Flight suitability
    bool isSafeForFlight() const;
    QString getSafetyWarnings() const;
    int getFlightSuitabilityScore() const; // 0-100
};

/**
 * @brief Service for fetching weather data
 *
 * Integrates with OpenWeatherMap API to provide:
 * - Current weather conditions
 * - Short-term forecasts
 * - Flight suitability analysis
 * - Wind direction/speed for mission planning
 */
class WeatherService : public QObject {
    Q_OBJECT

public:
    static WeatherService& instance();

    // API configuration
    void setApiKey(const QString& apiKey);
    QString apiKey() const { return m_apiKey; }
    bool isConfigured() const { return !m_apiKey.isEmpty(); }

    // Fetch weather data
    void fetchWeather(const Models::GeospatialCoordinate& location);
    void fetchWeather(double latitude, double longitude);

    // Get cached data
    WeatherData lastWeatherData() const { return m_lastData; }
    bool hasValidData() const;
    QDateTime lastUpdateTime() const { return m_lastUpdate; }

    QString lastError() const { return m_lastError; }

signals:
    void weatherDataReceived(const WeatherData& data);
    void weatherError(const QString& error);
    void fetchStarted();
    void fetchCompleted();

private slots:
    void onWeatherReplyFinished();
    void onForecastReplyFinished();

private:
    WeatherService();
    ~WeatherService();
    WeatherService(const WeatherService&) = delete;
    WeatherService& operator=(const WeatherService&) = delete;

    void parseWeatherData(const QByteArray& json);
    void parseForecastData(const QByteArray& json);

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    WeatherData m_lastData;
    QDateTime m_lastUpdate;
    QString m_lastError;

    // API endpoints
    static const QString API_BASE_URL;
    static const QString API_CURRENT_WEATHER;
    static const QString API_FORECAST;
};

} // namespace Core
} // namespace DroneMapper

#endif // WEATHERSERVICE_H
