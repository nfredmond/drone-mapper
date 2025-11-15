#include "CoordinateTransform.h"
#include <cmath>

namespace DroneMapper {
namespace Geospatial {

CoordinateTransform::CoordinateTransform()
    : m_projContext(nullptr)
{
    m_projContext = proj_context_create();
}

CoordinateTransform::~CoordinateTransform()
{
    if (m_projContext) {
        proj_context_destroy(m_projContext);
    }
}

bool CoordinateTransform::transformToProjected(
    const Models::GeospatialCoordinate& coord,
    const QString& targetCRS,
    double& x,
    double& y)
{
    QString sourceCRS = "EPSG:4326";  // WGS84

    PJ* transformer = proj_create_crs_to_crs(
        m_projContext,
        sourceCRS.toUtf8().constData(),
        targetCRS.toUtf8().constData(),
        nullptr
    );

    if (!transformer) {
        m_lastError = "Failed to create coordinate transformer";
        return false;
    }

    PJ_COORD input;
    input.lpz.lam = coord.longitude();
    input.lpz.phi = coord.latitude();
    input.lpz.z = coord.altitude();

    PJ_COORD output = proj_trans(transformer, PJ_FWD, input);

    x = output.xy.x;
    y = output.xy.y;

    proj_destroy(transformer);
    return true;
}

Models::GeospatialCoordinate CoordinateTransform::transformToGeographic(
    double x,
    double y,
    const QString& sourceCRS)
{
    QString targetCRS = "EPSG:4326";  // WGS84

    PJ* transformer = proj_create_crs_to_crs(
        m_projContext,
        sourceCRS.toUtf8().constData(),
        targetCRS.toUtf8().constData(),
        nullptr
    );

    if (!transformer) {
        m_lastError = "Failed to create coordinate transformer";
        return Models::GeospatialCoordinate();
    }

    PJ_COORD input;
    input.xy.x = x;
    input.xy.y = y;

    PJ_COORD output = proj_trans(transformer, PJ_FWD, input);

    proj_destroy(transformer);

    return Models::GeospatialCoordinate(output.lpz.phi, output.lpz.lam, 0.0);
}

QString CoordinateTransform::getUTMZone(const Models::GeospatialCoordinate& coord)
{
    int zone = static_cast<int>(std::floor((coord.longitude() + 180.0) / 6.0)) + 1;
    char hemisphere = (coord.latitude() >= 0) ? 'N' : 'S';

    // Return EPSG code for UTM zone
    int epsgCode = (hemisphere == 'N') ? (32600 + zone) : (32700 + zone);
    return QString("EPSG:%1").arg(epsgCode);
}

} // namespace Geospatial
} // namespace DroneMapper
