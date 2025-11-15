#ifndef SUNCALCULATOR_H
#define SUNCALCULATOR_H

#include "GeospatialCoordinate.h"
#include <QDateTime>
#include <QList>
#include <QPair>

namespace DroneMapper {
namespace Core {

/**
 * @brief Solar position and timing data
 */
struct SolarPosition {
    double altitude;      // Solar elevation angle (degrees, 0-90)
    double azimuth;       // Solar azimuth (degrees, 0-360)
    double zenith;        // Solar zenith angle (degrees, 0-180)
    QDateTime time;       // Time of position

    bool isDaylight() const { return altitude > 0.0; }
    bool isGoldenHour() const { return altitude > 0.0 && altitude < 6.0; }
    bool isOptimalForPhotogrammetry() const;
    QString getPhase() const;
};

/**
 * @brief Daily sun event times
 */
struct SunTimes {
    QDateTime sunrise;
    QDateTime sunset;
    QDateTime solarNoon;
    QDateTime civilDawn;     // Sun 6° below horizon
    QDateTime civilDusk;
    QDateTime nauticalDawn;  // Sun 12° below horizon
    QDateTime nauticalDusk;
    QDateTime astronomicalDawn; // Sun 18° below horizon
    QDateTime astronomicalDusk;
    QDateTime goldenHourMorningStart;
    QDateTime goldenHourMorningEnd;
    QDateTime goldenHourEveningStart;
    QDateTime goldenHourEveningEnd;

    double dayLength() const; // Hours
    QList<QPair<QDateTime, QDateTime>> getOptimalFlightWindows() const;
};

/**
 * @brief Calculator for solar position and optimal flight times
 *
 * Provides:
 * - Solar position (altitude, azimuth) for any time
 * - Sunrise/sunset times with twilight phases
 * - Golden hour calculations
 * - Optimal photography time windows
 * - Shadow length and direction estimation
 * - Solar intensity for exposure planning
 *
 * Based on algorithms from:
 * - NOAA Solar Calculator
 * - Jean Meeus' "Astronomical Algorithms"
 */
class SunCalculator {
public:
    SunCalculator();

    /**
     * @brief Calculate solar position for given location and time
     */
    static SolarPosition calculatePosition(
        const Models::GeospatialCoordinate& location,
        const QDateTime& time);

    static SolarPosition calculatePosition(
        double latitude,
        double longitude,
        const QDateTime& time);

    /**
     * @brief Calculate sun event times for a given date
     */
    static SunTimes calculateSunTimes(
        const Models::GeospatialCoordinate& location,
        const QDate& date);

    static SunTimes calculateSunTimes(
        double latitude,
        double longitude,
        const QDate& date);

    /**
     * @brief Get optimal flight windows for photogrammetry
     * @param location Survey location
     * @param date Mission date
     * @param minSunAngle Minimum sun elevation (default: 15°)
     * @param maxSunAngle Maximum sun elevation (default: 60°)
     * @return List of time windows (start, end)
     */
    static QList<QPair<QDateTime, QDateTime>> getOptimalFlightWindows(
        const Models::GeospatialCoordinate& location,
        const QDate& date,
        double minSunAngle = 15.0,
        double maxSunAngle = 60.0);

    /**
     * @brief Calculate shadow length for object height
     * @param objectHeight Height of object in meters
     * @param sunAltitude Solar elevation angle in degrees
     * @return Shadow length in meters
     */
    static double calculateShadowLength(double objectHeight, double sunAltitude);

    /**
     * @brief Estimate solar intensity (0-1 scale)
     * @param position Solar position
     * @return Intensity factor (1.0 = maximum at solar noon, clear sky)
     */
    static double calculateSolarIntensity(const SolarPosition& position);

    /**
     * @brief Get recommended flight time based on sun position
     * @param location Survey location
     * @param date Mission date
     * @return Recommended UTC time for flight
     */
    static QDateTime getRecommendedFlightTime(
        const Models::GeospatialCoordinate& location,
        const QDate& date);

private:
    // Low-level solar calculations
    static double calculateJulianDay(const QDateTime& dt);
    static double calculateJulianCentury(double julianDay);
    static double calculateGeomMeanLongSun(double t);
    static double calculateGeomMeanAnomalySun(double t);
    static double calculateEccentEarthOrbit(double t);
    static double calculateSunEqOfCenter(double t);
    static double calculateSunTrueLong(double t);
    static double calculateSunApparentLong(double t);
    static double calculateMeanObliqEcliptic(double t);
    static double calculateObliqCorrection(double t);
    static double calculateSunRtAscension(double t);
    static double calculateSunDeclination(double t);
    static double calculateEquationOfTime(double t);
    static double calculateHourAngleSunrise(double lat, double solarDec);
    static double calculateSolarNoonLST(double lon, double eqTime);
    static double calculateSunriseTimeUTC(double jd, double lat, double lon);
    static double calculateSunsetTimeUTC(double jd, double lat, double lon);

    // Helper functions
    static double degToRad(double deg) { return deg * M_PI / 180.0; }
    static double radToDeg(double rad) { return rad * 180.0 / M_PI; }
    static double normalizeDegrees(double deg);
};

} // namespace Core
} // namespace DroneMapper

#endif // SUNCALCULATOR_H
