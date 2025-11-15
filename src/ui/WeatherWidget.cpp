#include "WeatherWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QPalette>

namespace DroneMapper {
namespace UI {

WeatherWidget::WeatherWidget(QWidget *parent)
    : QWidget(parent)
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_autoRefreshTimer(new QTimer(this))
{
    setupUI();

    // Connect to weather service
    connect(&Core::WeatherService::instance(), &Core::WeatherService::weatherDataReceived,
            this, &WeatherWidget::onWeatherDataReceived);
    connect(&Core::WeatherService::instance(), &Core::WeatherService::weatherError,
            this, &WeatherWidget::onWeatherError);

    // Auto-refresh every 30 minutes
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &WeatherWidget::onAutoRefreshTimeout);
    m_autoRefreshTimer->start(1800000); // 30 minutes
}

WeatherWidget::~WeatherWidget()
{
}

void WeatherWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    // Title and refresh button
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(tr("<b>Weather Conditions</b>"));
    m_refreshButton = new QPushButton(tr("Refresh"));
    m_refreshButton->setMaximumWidth(80);
    connect(m_refreshButton, &QPushButton::clicked, this, &WeatherWidget::onRefreshClicked);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshButton);
    mainLayout->addLayout(headerLayout);

    // Flight suitability indicator
    QGroupBox *suitabilityGroup = new QGroupBox(tr("Flight Suitability"));
    QVBoxLayout *suitabilityLayout = new QVBoxLayout(suitabilityGroup);

    m_suitabilityBar = new QProgressBar();
    m_suitabilityBar->setRange(0, 100);
    m_suitabilityBar->setValue(0);
    m_suitabilityBar->setTextVisible(true);
    m_suitabilityBar->setFormat("%v%");

    m_suitabilityLabel = new QLabel(tr("No data"));
    m_suitabilityLabel->setWordWrap(true);

    suitabilityLayout->addWidget(m_suitabilityBar);
    suitabilityLayout->addWidget(m_suitabilityLabel);

    mainLayout->addWidget(suitabilityGroup);

    // Current conditions
    QGroupBox *conditionsGroup = new QGroupBox(tr("Current Conditions"));
    QGridLayout *conditionsLayout = new QGridLayout(conditionsGroup);

    m_descriptionLabel = new QLabel(tr("--"));
    m_descriptionLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
    conditionsLayout->addWidget(m_descriptionLabel, 0, 0, 1, 2);

    m_temperatureLabel = new QLabel(tr("Temperature: --"));
    m_feelsLikeLabel = new QLabel(tr("Feels like: --"));
    m_windLabel = new QLabel(tr("Wind: --"));
    m_windDirectionLabel = new QLabel(tr("Direction: --"));
    m_humidityLabel = new QLabel(tr("Humidity: --"));
    m_pressureLabel = new QLabel(tr("Pressure: --"));
    m_visibilityLabel = new QLabel(tr("Visibility: --"));
    m_cloudCoverLabel = new QLabel(tr("Clouds: --"));

    conditionsLayout->addWidget(m_temperatureLabel, 1, 0);
    conditionsLayout->addWidget(m_feelsLikeLabel, 1, 1);
    conditionsLayout->addWidget(m_windLabel, 2, 0);
    conditionsLayout->addWidget(m_windDirectionLabel, 2, 1);
    conditionsLayout->addWidget(m_humidityLabel, 3, 0);
    conditionsLayout->addWidget(m_pressureLabel, 3, 1);
    conditionsLayout->addWidget(m_visibilityLabel, 4, 0);
    conditionsLayout->addWidget(m_cloudCoverLabel, 4, 1);

    mainLayout->addWidget(conditionsGroup);

    // Sun times
    QGroupBox *sunGroup = new QGroupBox(tr("Sun Times"));
    QHBoxLayout *sunLayout = new QHBoxLayout(sunGroup);
    m_sunTimesLabel = new QLabel(tr("Sunrise: -- | Sunset: --"));
    sunLayout->addWidget(m_sunTimesLabel);
    mainLayout->addWidget(sunGroup);

    // Forecast
    QGroupBox *forecastGroup = new QGroupBox(tr("Next 3 Hours"));
    QVBoxLayout *forecastLayout = new QVBoxLayout(forecastGroup);
    m_forecastLabel = new QLabel(tr("--"));
    m_forecastLabel->setWordWrap(true);
    forecastLayout->addWidget(m_forecastLabel);
    mainLayout->addWidget(forecastGroup);

    // Warnings
    QGroupBox *warningsGroup = new QGroupBox(tr("Safety Warnings"));
    QVBoxLayout *warningsLayout = new QVBoxLayout(warningsGroup);
    m_warningsLabel = new QLabel(tr("No warnings"));
    m_warningsLabel->setWordWrap(true);
    m_warningsLabel->setStyleSheet("color: #2e7d32;"); // Green for safe
    warningsLayout->addWidget(m_warningsLabel);
    mainLayout->addWidget(warningsGroup);

    // Last update
    m_lastUpdateLabel = new QLabel(tr("Last update: Never"));
    m_lastUpdateLabel->setStyleSheet("font-size: 8pt; color: #666;");
    mainLayout->addWidget(m_lastUpdateLabel);

    mainLayout->addStretch();

    setMinimumWidth(280);
}

void WeatherWidget::setLocation(double latitude, double longitude)
{
    m_latitude = latitude;
    m_longitude = longitude;
    emit locationChanged(latitude, longitude);

    // Auto-refresh when location changes
    refreshWeather();
}

void WeatherWidget::refreshWeather()
{
    if (m_latitude == 0.0 && m_longitude == 0.0) {
        m_warningsLabel->setText(tr("No location set. Draw an area on the map first."));
        m_warningsLabel->setStyleSheet("color: #f57c00;"); // Orange
        return;
    }

    m_refreshButton->setEnabled(false);
    m_refreshButton->setText(tr("Loading..."));

    Core::WeatherService::instance().fetchWeather(m_latitude, m_longitude);
}

void WeatherWidget::onWeatherDataReceived(const Core::WeatherData& data)
{
    m_currentData = data;
    updateDisplay(data);

    m_refreshButton->setEnabled(true);
    m_refreshButton->setText(tr("Refresh"));

    m_lastUpdateLabel->setText(tr("Last update: %1")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

    emit weatherUpdated(data);
}

void WeatherWidget::onWeatherError(const QString& error)
{
    m_refreshButton->setEnabled(true);
    m_refreshButton->setText(tr("Refresh"));

    m_warningsLabel->setText(tr("Error: %1").arg(error));
    m_warningsLabel->setStyleSheet("color: #d32f2f;"); // Red for error

    if (error.contains("API key")) {
        m_suitabilityLabel->setText(
            tr("Weather API key not configured.\n"
               "Go to Edit ’ Settings to set your OpenWeatherMap API key.\n"
               "Free API keys available at: openweathermap.org/api"));
    }
}

void WeatherWidget::onRefreshClicked()
{
    refreshWeather();
}

void WeatherWidget::onAutoRefreshTimeout()
{
    // Auto-refresh only if we have valid location and existing data
    if ((m_latitude != 0.0 || m_longitude != 0.0) &&
        Core::WeatherService::instance().hasValidData()) {
        refreshWeather();
    }
}

void WeatherWidget::updateDisplay(const Core::WeatherData& data)
{
    // Description
    QString desc = data.description;
    desc[0] = desc[0].toUpper(); // Capitalize first letter
    m_descriptionLabel->setText(desc);

    // Temperature
    m_temperatureLabel->setText(tr("Temperature: %1°C").arg(data.temperature, 0, 'f', 1));
    m_feelsLikeLabel->setText(tr("Feels like: %1°C").arg(data.feelsLike, 0, 'f', 1));

    // Wind
    m_windLabel->setText(tr("Wind: %1 m/s").arg(data.windSpeed, 0, 'f', 1));
    if (data.windGust > 0.0) {
        m_windLabel->setText(m_windLabel->text() +
            tr(" (gusts %1 m/s)").arg(data.windGust, 0, 'f', 1));
    }
    m_windDirectionLabel->setText(tr("Direction: %1 (%2°)")
        .arg(getWindDirectionText(data.windDirection))
        .arg(data.windDirection, 0, 'f', 0));

    // Other conditions
    m_humidityLabel->setText(tr("Humidity: %1%").arg(data.humidity, 0, 'f', 0));
    m_pressureLabel->setText(tr("Pressure: %1 hPa").arg(data.pressure, 0, 'f', 0));
    m_visibilityLabel->setText(tr("Visibility: %1 km")
        .arg(data.visibility / 1000.0, 0, 'f', 1));
    m_cloudCoverLabel->setText(tr("Clouds: %1%").arg(data.cloudCover, 0, 'f', 0));

    // Sun times
    m_sunTimesLabel->setText(tr("Sunrise: %1 | Sunset: %2")
        .arg(formatTime(data.sunrise))
        .arg(formatTime(data.sunset)));

    // Forecast
    if (!data.forecastDesc.isEmpty()) {
        m_forecastLabel->setText(tr("Temperature: %1°C, Wind: %2 m/s\n%3")
            .arg(data.tempForecast, 0, 'f', 1)
            .arg(data.windForecast, 0, 'f', 1)
            .arg(data.forecastDesc));
    }

    // Flight suitability
    int score = data.getFlightSuitabilityScore();
    m_suitabilityBar->setValue(score);

    QPalette palette = m_suitabilityBar->palette();
    palette.setColor(QPalette::Highlight, getSuitabilityColor(score));
    m_suitabilityBar->setPalette(palette);

    QString suitabilityText;
    if (score >= 80) {
        suitabilityText = tr("EXCELLENT - Ideal conditions");
        m_suitabilityLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
    } else if (score >= 60) {
        suitabilityText = tr("GOOD - Safe to fly");
        m_suitabilityLabel->setStyleSheet("color: #388e3c;");
    } else if (score >= 40) {
        suitabilityText = tr("FAIR - Proceed with caution");
        m_suitabilityLabel->setStyleSheet("color: #f57c00; font-weight: bold;");
    } else if (score >= 20) {
        suitabilityText = tr("POOR - Not recommended");
        m_suitabilityLabel->setStyleSheet("color: #e64a19; font-weight: bold;");
    } else {
        suitabilityText = tr("UNSAFE - Do not fly!");
        m_suitabilityLabel->setStyleSheet("color: #d32f2f; font-weight: bold;");
    }
    m_suitabilityLabel->setText(suitabilityText);

    // Warnings
    QString warnings = data.getSafetyWarnings();
    m_warningsLabel->setText(warnings);

    if (data.isSafeForFlight()) {
        m_warningsLabel->setStyleSheet("color: #2e7d32;"); // Green
    } else {
        m_warningsLabel->setStyleSheet("color: #d32f2f; font-weight: bold;"); // Red
    }
}

QString WeatherWidget::getWindDirectionText(double degrees) const
{
    const char* directions[] = {
        "N", "NNE", "NE", "ENE",
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW",
        "W", "WNW", "NW", "NNW"
    };

    int index = static_cast<int>((degrees + 11.25) / 22.5) % 16;
    return QString(directions[index]);
}

QColor WeatherWidget::getSuitabilityColor(int score) const
{
    if (score >= 80) return QColor("#4caf50"); // Green
    if (score >= 60) return QColor("#8bc34a"); // Light green
    if (score >= 40) return QColor("#ff9800"); // Orange
    if (score >= 20) return QColor("#ff5722"); // Deep orange
    return QColor("#f44336"); // Red
}

QString WeatherWidget::formatTime(const QDateTime& dt) const
{
    return dt.toString("hh:mm");
}

} // namespace UI
} // namespace DroneMapper
