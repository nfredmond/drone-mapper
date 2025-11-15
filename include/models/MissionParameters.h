#ifndef MISSIONPARAMETERS_H
#define MISSIONPARAMETERS_H

#include <QString>

namespace DroneMapper {
namespace Models {

/**
 * @brief Mission parameters for flight planning
 */
class MissionParameters {
public:
    enum class CameraModel {
        DJI_Mini3,
        DJI_Mini3Pro,
        DJI_Air3,
        DJI_Mavic3,
        Custom
    };

    enum class FinishAction {
        ReturnToHome,
        Hover,
        Land,
        GoToFirstWaypoint
    };

    MissionParameters();

    // Flight parameters
    double flightAltitude() const { return m_flightAltitude; }
    double flightSpeed() const { return m_flightSpeed; }
    double pathSpacing() const { return m_pathSpacing; }
    double flightDirection() const { return m_flightDirection; }
    bool reversePath() const { return m_reversePath; }

    // Camera parameters
    CameraModel cameraModel() const { return m_cameraModel; }
    double frontOverlap() const { return m_frontOverlap; }
    double sideOverlap() const { return m_sideOverlap; }
    double cameraAngle() const { return m_cameraAngle; }
    double gimbalPitch() const { return m_gimbalPitch; }

    // Mission behavior
    FinishAction finishAction() const { return m_finishAction; }
    double takeoffAltitude() const { return m_takeoffAltitude; }
    double maxSpeed() const { return m_maxSpeed; }

    // Setters
    void setFlightAltitude(double altitude) { m_flightAltitude = altitude; }
    void setFlightSpeed(double speed) { m_flightSpeed = speed; }
    void setPathSpacing(double spacing) { m_pathSpacing = spacing; }
    void setFlightDirection(double direction) { m_flightDirection = direction; }
    void setReversePath(bool reverse) { m_reversePath = reverse; }

    void setCameraModel(CameraModel model) { m_cameraModel = model; }
    void setFrontOverlap(double overlap) { m_frontOverlap = overlap; }
    void setSideOverlap(double overlap) { m_sideOverlap = overlap; }
    void setCameraAngle(double angle) { m_cameraAngle = angle; }
    void setGimbalPitch(double pitch) { m_gimbalPitch = pitch; }

    void setFinishAction(FinishAction action) { m_finishAction = action; }
    void setTakeoffAltitude(double altitude) { m_takeoffAltitude = altitude; }
    void setMaxSpeed(double speed) { m_maxSpeed = speed; }

    // Calculate derived values
    double groundSampleDistance() const;  // GSD in cm/pixel
    double imageFootprintWidth() const;   // Meters
    double imageFootprintHeight() const;  // Meters

private:
    // Flight parameters
    double m_flightAltitude;      // Meters
    double m_flightSpeed;         // m/s
    double m_pathSpacing;         // Meters between flight lines
    double m_flightDirection;     // Degrees (0-360)
    bool m_reversePath;

    // Camera parameters
    CameraModel m_cameraModel;
    double m_frontOverlap;        // Percentage (0-100)
    double m_sideOverlap;         // Percentage (0-100)
    double m_cameraAngle;         // Degrees
    double m_gimbalPitch;         // Degrees (-90 to 0)

    // Mission behavior
    FinishAction m_finishAction;
    double m_takeoffAltitude;     // Meters
    double m_maxSpeed;            // m/s
};

} // namespace Models
} // namespace DroneMapper

#endif // MISSIONPARAMETERS_H
