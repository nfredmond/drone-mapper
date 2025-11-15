#ifndef WPMLWRITER_H
#define WPMLWRITER_H

#include "FlightPlan.h"
#include <QString>
#include <QXmlStreamWriter>

namespace DroneMapper {
namespace Geospatial {

/**
 * @brief Generates WPML (WayPoint Markup Language) XML for DJI drones
 *
 * Implements DJI's WPML specification for waypoint missions compatible with:
 * - DJI Mini 3 / Mini 3 Pro
 * - DJI Air 3
 * - DJI Mavic 3 (consumer and enterprise)
 */
class WPMLWriter {
public:
    enum class DroneModel {
        Mini3,
        Mini3Pro,
        Air3,
        Mavic3,
        Mavic3Pro
    };

    WPMLWriter();

    /**
     * @brief Generate WPML XML from a flight plan
     * @param plan The flight plan to convert
     * @param droneModel Target drone model
     * @return WPML XML as QString
     */
    QString generate(const Models::FlightPlan& plan, DroneModel droneModel);

    /**
     * @brief Validate generated WPML against specification
     * @param wpml The WPML XML to validate
     * @return true if valid, false otherwise
     */
    bool validate(const QString& wpml);

    QString lastError() const { return m_lastError; }

private:
    void writeMissionConfig(QXmlStreamWriter& writer,
                          const Models::FlightPlan& plan,
                          DroneModel droneModel);

    void writeWaypoints(QXmlStreamWriter& writer,
                       const Models::FlightPlan& plan);

    void writeWaypoint(QXmlStreamWriter& writer,
                      const Models::Waypoint& waypoint,
                      int index);

    void writeActions(QXmlStreamWriter& writer,
                     const Models::Waypoint& waypoint);

    QString getDroneEnumValue(DroneModel model);
    QString getFinishAction(Models::MissionParameters::FinishAction action);

    QString m_lastError;
};

} // namespace Geospatial
} // namespace DroneMapper

#endif // WPMLWRITER_H
