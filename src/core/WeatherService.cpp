#include "WeatherService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <cmath>

namespace DroneMapper {
namespace Core {

const QString WeatherService::API_BASE_URL = "https://api.openweathermap.org/data/2.5";
const QString WeatherService::API_CURRENT_WEATHER = "/weather";
const QString WeatherService::API_FORECAST = "/forecast";

bool WeatherData::isSafeForFlight() const
{
    // Conservative safety criteria for drone operations
    if (windSpeed > 12.0) return false;         // > 12 m/s (26 mph) too windy
    if (windGust > 15.0) return false;          // Gusts > 15 m/s dangerous
    if (precipitation > 0.1) return false;      // Any significant precipitation
    if (visibility < 5000.0) return false;      // < 5km visibility
    if (temperature < -10.0) return false;      // Too cold (battery issues)
    if (temperature > 40.0) return false;       // Too hot

    return true;
}

QString WeatherData::getSafetyWarnings() const
{
    QStringList warnings;

    if (windSpeed > 12.0) {
        warnings << QString("High wind speed: %1 m/s (max safe: 12 m/s)")
            .arg(windSpeed, 0, 'f', 1);
    } else if (windSpeed > 10.0) {
        warnings << QString("Moderate wind: %1 m/s - fly with caution")
            .arg(windSpeed, 0, 'f', 1);
    }

    if (windGust > 15.0) {
        warnings << QString("Dangerous wind gusts: %1 m/s").arg(windGust, 0, 'f', 1);
    } else if (windGust > 12.0) {
        warnings << QString("Strong gusts: %1 m/s - increased risk")
            .arg(windGust, 0, 'f', 1);
    }

    if (precipitation > 0.1) {
        warnings << QString("Precipitation: %1 mm - do not fly").arg(precipitation, 0, 'f', 1);
    }

    if (visibility < 5000.0) {
        warnings << QString("Low visibility: %1 m (min safe: 5000 m)")
            .arg(visibility, 0, 'f', 0);
    }

    if (temperature < -10.0) {
        warnings << "Temperature too low - battery performance degraded";
    } else if (temperature > 40.0) {
        warnings << "Temperature too high - risk of overheating";
    }

    if (cloudCover > 80.0) {
        warnings << "Heavy cloud cover - poor lighting for photogrammetry";
    }

    if (warnings.isEmpty()) {
        return "All conditions favorable for flight";
    }

    return warnings.join("\n");
}

int WeatherData::getFlightSuitabilityScore() const
{
    int score = 100;

    // Wind penalties
    if (windSpeed > 12.0) score -= 100;
    else if (windSpeed > 10.0) score -= 30;
    else if (windSpeed > 8.0) score -= 15;
    else if (windSpeed > 6.0) score -= 5;

    // Gust penalties
    if (windGust > 15.0) score -= 100;
    else if (windGust > 12.0) score -= 40;
    else if (windGust > 10.0) score -= 20;

    // Precipitation penalties
    if (precipitation > 0.5) score -= 100;
    else if (precipitation > 0.1) score -= 50;

    // Visibility penalties
    if (visibility < 5000.0) score -= 50;
    else if (visibility < 10000.0) score -= 20;

    // Temperature penalties
    if (temperature < -10.0 || temperature > 40.0) score -= 100;
    else if (temperature < 0.0 || temperature > 35.0) score -= 30;
    else if (temperature < 5.0 || temperature > 30.0) score -= 10;

    // Cloud cover (affects photogrammetry quality)
    if (cloudCover > 90.0) score -= 30;
    else if (cloudCover > 70.0) score -= 15;
    else if (cloudCover < 20.0) score += 10; // Bonus for clear skies

    return qMax(0, qMin(100, score));
}

WeatherService& WeatherService::instance()
{
    static WeatherService instance;
    return instance;
}

WeatherService::WeatherService()
    : QObject(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Initialize with empty data
    m_lastData = WeatherData();
}

WeatherService::~WeatherService()
{
}

void WeatherService::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
}

void WeatherService::fetchWeather(const Models::GeospatialCoordinate& location)
{
    fetchWeather(location.latitude(), location.longitude());
}

void WeatherService::fetchWeather(double latitude, double longitude)
{
    if (!isConfigured()) {
        m_lastError = "Weather API key not configured. Please set API key in settings.";
        emit weatherError(m_lastError);
        return;
    }

    emit fetchStarted();
    m_lastError.clear();

    // Build current weather API URL
    QUrl url(API_BASE_URL + API_CURRENT_WEATHER);
    QUrlQuery query;
    query.addQueryItem("lat", QString::number(latitude, 'f', 6));
    query.addQueryItem("lon", QString::number(longitude, 'f', 6));
    query.addQueryItem("appid", m_apiKey);
    query.addQueryItem("units", "metric");
    url.setQuery(query);

    // Fetch current weather
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "DroneMapper/1.0");
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &WeatherService::onWeatherReplyFinished);

    // Also fetch forecast for next 3 hours
    QUrl forecastUrl(API_BASE_URL + API_FORECAST);
    QUrlQuery forecastQuery;
    forecastQuery.addQueryItem("lat", QString::number(latitude, 'f', 6));
    forecastQuery.addQueryItem("lon", QString::number(longitude, 'f', 6));
    forecastQuery.addQueryItem("appid", m_apiKey);
    forecastQuery.addQueryItem("units", "metric");
    forecastQuery.addQueryItem("cnt", "1"); // Just next 3 hours
    forecastUrl.setQuery(forecastQuery);

    QNetworkRequest forecastRequest(forecastUrl);
    forecastRequest.setHeader(QNetworkRequest::UserAgentHeader, "DroneMapper/1.0");
    QNetworkReply *forecastReply = m_networkManager->get(forecastRequest);
    connect(forecastReply, &QNetworkReply::finished, this, &WeatherService::onForecastReplyFinished);
}

void WeatherService::onWeatherReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = QString("Network error: %1").arg(reply->errorString());
        emit weatherError(m_lastError);
        emit fetchCompleted();
        return;
    }

    QByteArray data = reply->readAll();
    parseWeatherData(data);
}

void WeatherService::onForecastReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        parseForecastData(data);
    }
    // Don't emit error for forecast - it's optional
}

void WeatherService::parseWeatherData(const QByteArray& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull() || !doc.isObject()) {
        m_lastError = "Invalid JSON response from weather API";
        emit weatherError(m_lastError);
        emit fetchCompleted();
        return;
    }

    QJsonObject root = doc.object();

    // Check for API error
    if (root.contains("cod") && root["cod"].toInt() != 200) {
        m_lastError = QString("API error: %1").arg(root["message"].toString());
        emit weatherError(m_lastError);
        emit fetchCompleted();
        return;
    }

    WeatherData data;

    // Main weather data
    QJsonObject main = root["main"].toObject();
    data.temperature = main["temp"].toDouble();
    data.feelsLike = main["feels_like"].toDouble();
    data.humidity = main["humidity"].toDouble();
    data.pressure = main["pressure"].toDouble();

    // Wind data
    QJsonObject wind = root["wind"].toObject();
    data.windSpeed = wind["speed"].toDouble();
    data.windDirection = wind["deg"].toDouble();
    data.windGust = wind["gust"].toDouble(0.0);

    // Visibility
    data.visibility = root["visibility"].toDouble(10000.0);

    // Clouds
    QJsonObject clouds = root["clouds"].toObject();
    data.cloudCover = clouds["all"].toDouble();

    // Precipitation (rain/snow)
    data.precipitation = 0.0;
    if (root.contains("rain")) {
        QJsonObject rain = root["rain"].toObject();
        data.precipitation = rain["1h"].toDouble(0.0);
    }
    if (root.contains("snow")) {
        QJsonObject snow = root["snow"].toObject();
        data.precipitation += snow["1h"].toDouble(0.0);
    }

    // Weather description
    QJsonArray weather = root["weather"].toArray();
    if (!weather.isEmpty()) {
        QJsonObject weatherObj = weather[0].toObject();
        data.description = weatherObj["description"].toString();
        data.icon = weatherObj["icon"].toString();
    }

    // Sun times
    QJsonObject sys = root["sys"].toObject();
    data.sunrise = QDateTime::fromSecsSinceEpoch(sys["sunrise"].toInteger());
    data.sunset = QDateTime::fromSecsSinceEpoch(sys["sunset"].toInteger());

    m_lastData = data;
    m_lastUpdate = QDateTime::currentDateTime();

    emit weatherDataReceived(data);
    emit fetchCompleted();
}

void WeatherService::parseForecastData(const QByteArray& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonArray list = root["list"].toArray();

    if (!list.isEmpty()) {
        QJsonObject forecast = list[0].toObject();
        QJsonObject main = forecast["main"].toObject();
        QJsonObject wind = forecast["wind"].toObject();

        m_lastData.tempForecast = main["temp"].toDouble();
        m_lastData.windForecast = wind["speed"].toDouble();

        QJsonArray weather = forecast["weather"].toArray();
        if (!weather.isEmpty()) {
            m_lastData.forecastDesc = weather[0].toObject()["description"].toString();
        }
    }
}

bool WeatherService::hasValidData() const
{
    if (!m_lastUpdate.isValid()) return false;

    // Data is valid for 30 minutes
    return m_lastUpdate.secsTo(QDateTime::currentDateTime()) < 1800;
}

} // namespace Core
} // namespace DroneMapper
