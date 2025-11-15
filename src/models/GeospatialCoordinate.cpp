#include "GeospatialCoordinate.h"
#include <cmath>
#include <QStringList>

namespace DroneMapper {
namespace Models {

GeospatialCoordinate::GeospatialCoordinate()
    : m_latitude(0.0), m_longitude(0.0), m_altitude(0.0)
{
}

GeospatialCoordinate::GeospatialCoordinate(double latitude, double longitude, double altitude)
    : m_latitude(latitude), m_longitude(longitude), m_altitude(altitude)
{
}

QString GeospatialCoordinate::toDecimalDegrees() const
{
    return QString("%1, %2 @ %3m")
        .arg(m_latitude, 0, 'f', 6)
        .arg(m_longitude, 0, 'f', 6)
        .arg(m_altitude, 0, 'f', 1);
}

QString GeospatialCoordinate::toDMS() const
{
    auto toDMS = [](double decimal, bool isLatitude) -> QString {
        char dir;
        if (isLatitude) {
            dir = (decimal >= 0) ? 'N' : 'S';
        } else {
            dir = (decimal >= 0) ? 'E' : 'W';
        }

        decimal = std::abs(decimal);
        int degrees = static_cast<int>(decimal);
        double minutesDecimal = (decimal - degrees) * 60.0;
        int minutes = static_cast<int>(minutesDecimal);
        double seconds = (minutesDecimal - minutes) * 60.0;

        return QString("%1°%2'%3\"%4")
            .arg(degrees)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 0, 'f', 2)
            .arg(dir);
    };

    return QString("%1 %2")
        .arg(toDMS(m_latitude, true))
        .arg(toDMS(m_longitude, false));
}

QString GeospatialCoordinate::toUTM() const
{
    // Simplified UTM conversion - in production use PROJ library
    return QString("UTM: %1, %2").arg(m_latitude).arg(m_longitude);
}

bool GeospatialCoordinate::isValid() const
{
    return (m_latitude >= -90.0 && m_latitude <= 90.0) &&
           (m_longitude >= -180.0 && m_longitude <= 180.0);
}

} // namespace Models
} // namespace DroneMapper
