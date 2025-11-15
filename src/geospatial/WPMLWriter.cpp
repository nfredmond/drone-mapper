#include "WPMLWriter.h"
#include <QXmlStreamWriter>
#include <QBuffer>

namespace DroneMapper {
namespace Geospatial {

WPMLWriter::WPMLWriter()
{
}

QString WPMLWriter::generate(const Models::FlightPlan& plan, DroneModel droneModel)
{
    QString wpml;
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeStartDocument();
    writer.writeDefaultNamespace("http://www.dji.com/wpmz/1.0.0");
    writer.writeNamespace("http://www.w3.org/2005/Atom", "atom");

    writer.writeStartElement("kml");
    writer.writeAttribute("xmlns", "http://www.opengis.net/kml/2.2");

    writer.writeStartElement("Document");

    // Write mission configuration
    writeMissionConfig(writer, plan, droneModel);

    // Write folder containing waypoints
    writer.writeStartElement("Folder");

    // Template metadata
    writer.writeStartElement("wpml:templateId");
    writer.writeCharacters("0");
    writer.writeEndElement();

    writer.writeStartElement("wpml:executeHeightMode");
    writer.writeCharacters("relativeToStartPoint");
    writer.writeEndElement();

    writer.writeStartElement("wpml:waylineId");
    writer.writeCharacters("0");
    writer.writeEndElement();

    writer.writeStartElement("wpml:distance");
    writer.writeCharacters(QString::number(plan.totalDistance(), 'f', 2));
    writer.writeEndElement();

    writer.writeStartElement("wpml:duration");
    writer.writeCharacters(QString::number(plan.estimatedFlightTime() * 60)); // Convert to seconds
    writer.writeEndElement();

    writer.writeStartElement("wpml:autoFlightSpeed");
    writer.writeCharacters(QString::number(plan.parameters().flightSpeed(), 'f', 1));
    writer.writeEndElement();

    // Write waypoints
    writeWaypoints(writer, plan);

    writer.writeEndElement(); // Folder
    writer.writeEndElement(); // Document
    writer.writeEndElement(); // kml
    writer.writeEndDocument();

    wpml = QString::fromUtf8(buffer.data());
    return wpml;
}

void WPMLWriter::writeMissionConfig(QXmlStreamWriter& writer,
                                    const Models::FlightPlan& plan,
                                    DroneModel droneModel)
{
    writer.writeStartElement("wpml:missionConfig");

    // Fly to wayline mode
    writer.writeStartElement("wpml:flyToWaylineMode");
    writer.writeCharacters("safely");
    writer.writeEndElement();

    // Finish action
    writer.writeStartElement("wpml:finishAction");
    writer.writeCharacters(getFinishAction(plan.parameters().finishAction()));
    writer.writeEndElement();

    // Exit on RC signal lost
    writer.writeStartElement("wpml:exitOnRCLost");
    writer.writeCharacters("executeLostAction");
    writer.writeEndElement();

    // Takeoff security height
    writer.writeStartElement("wpml:takeOffSecurityHeight");
    writer.writeCharacters(QString::number(plan.parameters().takeoffAltitude(), 'f', 1));
    writer.writeEndElement();

    // Global transitional speed
    writer.writeStartElement("wpml:globalTransitionalSpeed");
    writer.writeCharacters(QString::number(plan.parameters().flightSpeed(), 'f', 1));
    writer.writeEndElement();

    // Drone info
    writer.writeStartElement("wpml:droneInfo");
    writer.writeStartElement("wpml:droneEnumValue");
    writer.writeCharacters(getDroneEnumValue(droneModel));
    writer.writeEndElement();
    writer.writeStartElement("wpml:droneSubEnumValue");
    writer.writeCharacters("0");
    writer.writeEndElement();
    writer.writeEndElement(); // droneInfo

    // Payload info (camera)
    writer.writeStartElement("wpml:payloadInfo");
    writer.writeStartElement("wpml:payloadEnumValue");
    writer.writeCharacters("52");  // Default camera
    writer.writeEndElement();
    writer.writeStartElement("wpml:payloadSubEnumValue");
    writer.writeCharacters("0");
    writer.writeEndElement();
    writer.writeStartElement("wpml:payloadPositionIndex");
    writer.writeCharacters("0");
    writer.writeEndElement();
    writer.writeEndElement(); // payloadInfo

    writer.writeEndElement(); // missionConfig
}

void WPMLWriter::writeWaypoints(QXmlStreamWriter& writer, const Models::FlightPlan& plan)
{
    const auto& waypoints = plan.waypoints();

    for (int i = 0; i < waypoints.count(); ++i) {
        writeWaypoint(writer, waypoints[i], i);
    }
}

void WPMLWriter::writeWaypoint(QXmlStreamWriter& writer,
                              const Models::Waypoint& waypoint,
                              int index)
{
    writer.writeStartElement("Placemark");

    // Point with coordinates
    writer.writeStartElement("Point");
    const auto& coord = waypoint.coordinate();
    QString coordinates = QString("%1,%2,%3")
        .arg(coord.longitude(), 0, 'f', 8)
        .arg(coord.latitude(), 0, 'f', 8)
        .arg(coord.altitude(), 0, 'f', 2);
    writer.writeTextElement("coordinates", coordinates);
    writer.writeEndElement(); // Point

    // Waypoint index
    writer.writeStartElement("wpml:index");
    writer.writeCharacters(QString::number(index));
    writer.writeEndElement();

    // Execute height
    writer.writeStartElement("wpml:executeHeight");
    writer.writeCharacters(QString::number(coord.altitude(), 'f', 2));
    writer.writeEndElement();

    // Waypoint speed
    writer.writeStartElement("wpml:waypointSpeed");
    writer.writeCharacters(QString::number(waypoint.speed(), 'f', 1));
    writer.writeEndElement();

    // Heading parameter
    writer.writeStartElement("wpml:waypointHeadingParam");
    writer.writeStartElement("wpml:waypointHeadingMode");

    switch (waypoint.headingMode()) {
        case Models::Waypoint::HeadingMode::Auto:
            writer.writeCharacters("followWayline");
            break;
        case Models::Waypoint::HeadingMode::Fixed:
            writer.writeCharacters("fixed");
            break;
        case Models::Waypoint::HeadingMode::PointOfInterest:
            writer.writeCharacters("towardPOI");
            break;
        default:
            writer.writeCharacters("followWayline");
            break;
    }
    writer.writeEndElement();

    if (waypoint.headingMode() == Models::Waypoint::HeadingMode::Fixed) {
        writer.writeStartElement("wpml:waypointHeadingAngle");
        writer.writeCharacters(QString::number(waypoint.heading(), 'f', 1));
        writer.writeEndElement();
    }

    writer.writeEndElement(); // waypointHeadingParam

    // Turn mode
    writer.writeStartElement("wpml:waypointTurnParam");
    writer.writeStartElement("wpml:waypointTurnMode");
    writer.writeCharacters("coordinateTurn");
    writer.writeEndElement();
    writer.writeStartElement("wpml:waypointTurnDampingDist");
    writer.writeCharacters("0.2");
    writer.writeEndElement();
    writer.writeEndElement(); // waypointTurnParam

    // Actions at waypoint
    if (!waypoint.actions().isEmpty()) {
        writeActions(writer, waypoint);
    }

    writer.writeEndElement(); // Placemark
}

void WPMLWriter::writeActions(QXmlStreamWriter& writer, const Models::Waypoint& waypoint)
{
    writer.writeStartElement("wpml:actionGroup");
    writer.writeStartElement("wpml:actionGroupId");
    writer.writeCharacters("0");
    writer.writeEndElement();

    writer.writeStartElement("wpml:actionGroupStartIndex");
    writer.writeCharacters(QString::number(waypoint.waypointNumber()));
    writer.writeEndElement();

    writer.writeStartElement("wpml:actionGroupEndIndex");
    writer.writeCharacters(QString::number(waypoint.waypointNumber()));
    writer.writeEndElement();

    writer.writeStartElement("wpml:actionGroupMode");
    writer.writeCharacters("sequence");
    writer.writeEndElement();

    writer.writeStartElement("wpml:actionTrigger");
    writer.writeStartElement("wpml:actionTriggerType");
    writer.writeCharacters("reachPoint");
    writer.writeEndElement();
    writer.writeEndElement();

    for (const auto& action : waypoint.actions()) {
        writer.writeStartElement("wpml:action");
        writer.writeStartElement("wpml:actionId");
        writer.writeCharacters("0");
        writer.writeEndElement();

        writer.writeStartElement("wpml:actionActuatorFunc");
        switch (action) {
            case Models::Waypoint::Action::TakePhoto:
                writer.writeCharacters("takePhoto");
                break;
            case Models::Waypoint::Action::StartVideo:
                writer.writeCharacters("startRecord");
                break;
            case Models::Waypoint::Action::StopVideo:
                writer.writeCharacters("stopRecord");
                break;
            case Models::Waypoint::Action::Hover:
                writer.writeCharacters("hover");
                writer.writeEndElement();
                writer.writeStartElement("wpml:actionActuatorFuncParam");
                writer.writeStartElement("wpml:hoverTime");
                writer.writeCharacters(QString::number(waypoint.hoverTime()));
                writer.writeEndElement();
                writer.writeEndElement();
                break;
            default:
                writer.writeCharacters("takePhoto");
                break;
        }
        writer.writeEndElement(); // actionActuatorFunc

        writer.writeEndElement(); // action
    }

    writer.writeEndElement(); // actionGroup
}

QString WPMLWriter::getDroneEnumValue(DroneModel model)
{
    switch (model) {
        case DroneModel::Mini3:
            return "77";  // DJI Mini 3
        case DroneModel::Mini3Pro:
            return "67";  // DJI Mini 3 Pro
        case DroneModel::Air3:
            return "91";  // DJI Air 3
        case DroneModel::Mavic3:
            return "60";  // DJI Mavic 3
        case DroneModel::Mavic3Pro:
            return "89";  // DJI Mavic 3 Pro
        default:
            return "67";
    }
}

QString WPMLWriter::getFinishAction(Models::MissionParameters::FinishAction action)
{
    switch (action) {
        case Models::MissionParameters::FinishAction::ReturnToHome:
            return "goHome";
        case Models::MissionParameters::FinishAction::Hover:
            return "hover";
        case Models::MissionParameters::FinishAction::Land:
            return "autoLand";
        case Models::MissionParameters::FinishAction::GoToFirstWaypoint:
            return "goToFirstWaypoint";
        default:
            return "goHome";
    }
}

bool WPMLWriter::validate(const QString& wpml)
{
    // Basic validation - check for required elements
    if (!wpml.contains("wpml:missionConfig")) {
        m_lastError = "Missing mission configuration";
        return false;
    }

    if (!wpml.contains("Placemark")) {
        m_lastError = "No waypoints found";
        return false;
    }

    return true;
}

} // namespace Geospatial
} // namespace DroneMapper
