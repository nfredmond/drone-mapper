#ifndef COORDINATETRANSFORM_H
#define COORDINATETRANSFORM_H

#include "GeospatialCoordinate.h"
#include <QString>
#include <proj.h>

namespace DroneMapper {
namespace Geospatial {

/**
 * @brief Handles coordinate system transformations using PROJ library
 */
class CoordinateTransform {
public:
    CoordinateTransform();
    ~CoordinateTransform();

    /**
     * @brief Transform coordinate from WGS84 to target CRS
     * @param coord Input coordinate in WGS84
     * @param targetCRS Target coordinate reference system (EPSG code or proj string)
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @return true if successful
     */
    bool transformToProjected(
        const Models::GeospatialCoordinate& coord,
        const QString& targetCRS,
        double& x,
        double& y);

    /**
     * @brief Transform from projected CRS to WGS84
     * @param x Input X coordinate
     * @param y Input Y coordinate
     * @param sourceCRS Source coordinate reference system
     * @return Geospatial coordinate in WGS84
     */
    Models::GeospatialCoordinate transformToGeographic(
        double x,
        double y,
        const QString& sourceCRS);

    /**
     * @brief Get UTM zone for a coordinate
     * @param coord Coordinate to check
     * @return EPSG code for appropriate UTM zone
     */
    QString getUTMZone(const Models::GeospatialCoordinate& coord);

    QString lastError() const { return m_lastError; }

private:
    PJ_CONTEXT* m_projContext;
    QString m_lastError;
};

} // namespace Geospatial
} // namespace DroneMapper

#endif // COORDINATETRANSFORM_H
