#ifndef GEOSPATIALCOORDINATE_H
#define GEOSPATIALCOORDINATE_H

#include <QString>

namespace DroneMapper {
namespace Models {

/**
 * @brief Represents a geospatial coordinate with latitude, longitude, and altitude
 */
class GeospatialCoordinate {
public:
    GeospatialCoordinate();
    GeospatialCoordinate(double latitude, double longitude, double altitude = 0.0);

    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    double altitude() const { return m_altitude; }

    void setLatitude(double latitude) { m_latitude = latitude; }
    void setLongitude(double longitude) { m_longitude = longitude; }
    void setAltitude(double altitude) { m_altitude = altitude; }

    // Convert to various formats
    QString toDecimalDegrees() const;
    QString toDMS() const;  // Degrees, Minutes, Seconds
    QString toUTM() const;

    bool isValid() const;

private:
    double m_latitude;   // Decimal degrees (-90 to 90)
    double m_longitude;  // Decimal degrees (-180 to 180)
    double m_altitude;   // Meters above sea level
};

} // namespace Models
} // namespace DroneMapper

#endif // GEOSPATIALCOORDINATE_H
