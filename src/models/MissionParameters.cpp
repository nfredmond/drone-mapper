#include "MissionParameters.h"
#include <cmath>

namespace DroneMapper {
namespace Models {

MissionParameters::MissionParameters()
    : m_flightAltitude(75.0)
    , m_flightSpeed(8.0)
    , m_pathSpacing(50.0)
    , m_flightDirection(0.0)
    , m_reversePath(false)
    , m_cameraModel(CameraModel::DJI_Mini3Pro)
    , m_frontOverlap(75.0)
    , m_sideOverlap(65.0)
    , m_cameraAngle(90.0)
    , m_gimbalPitch(-90.0)
    , m_finishAction(FinishAction::ReturnToHome)
    , m_takeoffAltitude(25.0)
    , m_maxSpeed(15.0)
{
}

double MissionParameters::groundSampleDistance() const
{
    // GSD calculation: (sensor width * altitude * 100) / (focal length * image width)
    // Simplified for DJI Mini 3 Pro: 1/1.3" sensor, 24mm equivalent
    // This is approximate - real implementation would use precise camera parameters

    double sensorWidth = 6.3; // mm for 1/1.3" sensor
    double focalLength = 6.72; // mm actual focal length (24mm equivalent)
    double imageWidth = 4000; // pixels

    double gsd = (sensorWidth * m_flightAltitude * 100.0) / (focalLength * imageWidth);
    return gsd; // cm/pixel
}

double MissionParameters::imageFootprintWidth() const
{
    // Calculate footprint based on sensor and altitude
    double sensorWidth = 6.3; // mm
    double focalLength = 6.72; // mm
    double footprintWidth = (sensorWidth * m_flightAltitude) / focalLength;
    return footprintWidth;
}

double MissionParameters::imageFootprintHeight() const
{
    // Calculate footprint based on sensor and altitude
    double sensorHeight = 4.7; // mm (3:4 ratio)
    double focalLength = 6.72; // mm
    double footprintHeight = (sensorHeight * m_flightAltitude) / focalLength;
    return footprintHeight;
}

} // namespace Models
} // namespace DroneMapper
