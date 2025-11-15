#include "KMZGenerator.h"
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QProcess>
#include <QXmlStreamWriter>
#include <QBuffer>

namespace DroneMapper {
namespace Geospatial {

KMZGenerator::KMZGenerator()
{
}

bool KMZGenerator::generate(const Models::FlightPlan& plan,
                           const QString& outputPath,
                           WPMLWriter::DroneModel droneModel)
{
    return generateWithVisualization(plan, outputPath, droneModel, true);
}

bool KMZGenerator::generateWithVisualization(const Models::FlightPlan& plan,
                                            const QString& outputPath,
                                            WPMLWriter::DroneModel droneModel,
                                            bool includeVisualization)
{
    // Generate WPML content
    QString wpml = m_wpmlWriter.generate(plan, droneModel);
    if (wpml.isEmpty()) {
        m_lastError = "Failed to generate WPML: " + m_wpmlWriter.lastError();
        return false;
    }

    // Generate visualization KML if requested
    QString kml;
    if (includeVisualization) {
        kml = generateKML(plan);
    }

    // Create KMZ file
    if (!createKMZ(wpml, kml, outputPath)) {
        return false;
    }

    return true;
}

bool KMZGenerator::createKMZ(const QString& wpmlContent,
                            const QString& kmlContent,
                            const QString& outputPath)
{
    // Create temporary directory for KMZ contents
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        m_lastError = "Failed to create temporary directory";
        return false;
    }

    // Write waylines.wpml
    QString wpmlPath = tempDir.filePath("waylines.wpml");
    QFile wpmlFile(wpmlPath);
    if (!wpmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Failed to write waylines.wpml";
        return false;
    }
    wpmlFile.write(wpmlContent.toUtf8());
    wpmlFile.close();

    // Write template.kml if provided
    if (!kmlContent.isEmpty()) {
        QString kmlPath = tempDir.filePath("template.kml");
        QFile kmlFile(kmlPath);
        if (!kmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_lastError = "Failed to write template.kml";
            return false;
        }
        kmlFile.write(kmlContent.toUtf8());
        kmlFile.close();
    }

    // Create res directory (for waypoint markers if needed)
    QDir resDir(tempDir.filePath("res"));
    resDir.mkpath(".");

    // Create KMZ (ZIP archive) using system zip command
    QProcess zipProcess;
    QStringList args;
    args << "-r" << outputPath << ".";

    zipProcess.setWorkingDirectory(tempDir.path());
    zipProcess.start("zip", args);

    if (!zipProcess.waitForFinished(30000)) {
        m_lastError = "Zip process timeout";
        return false;
    }

    if (zipProcess.exitCode() != 0) {
        m_lastError = "Failed to create KMZ: " + zipProcess.readAllStandardError();
        return false;
    }

    return true;
}

QString KMZGenerator::generateKML(const Models::FlightPlan& plan)
{
    QString kml;
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeStartDocument();
    writer.writeStartElement("kml");
    writer.writeAttribute("xmlns", "http://www.opengis.net/kml/2.2");

    writer.writeStartElement("Document");
    writer.writeTextElement("name", plan.name());
    writer.writeTextElement("description", plan.description());

    // Style for waypoint markers
    writer.writeStartElement("Style");
    writer.writeAttribute("id", "waypointStyle");
    writer.writeStartElement("IconStyle");
    writer.writeTextElement("scale", "1.0");
    writer.writeStartElement("Icon");
    writer.writeTextElement("href", "http://maps.google.com/mapfiles/kml/shapes/placemark_circle.png");
    writer.writeEndElement(); // Icon
    writer.writeEndElement(); // IconStyle
    writer.writeEndElement(); // Style

    // Style for flight path
    writer.writeStartElement("Style");
    writer.writeAttribute("id", "pathStyle");
    writer.writeStartElement("LineStyle");
    writer.writeTextElement("color", "ff0000ff");  // Red line
    writer.writeTextElement("width", "3");
    writer.writeEndElement();
    writer.writeEndElement();

    // Flight path line
    writer.writeStartElement("Placemark");
    writer.writeTextElement("name", "Flight Path");
    writer.writeTextElement("styleUrl", "#pathStyle");
    writer.writeStartElement("LineString");
    writer.writeTextElement("altitudeMode", "relativeToGround");

    QString coordinates;
    const auto& waypoints = plan.waypoints();
    for (const auto& wp : waypoints) {
        const auto& coord = wp.coordinate();
        coordinates += QString("%1,%2,%3\n")
            .arg(coord.longitude(), 0, 'f', 8)
            .arg(coord.latitude(), 0, 'f', 8)
            .arg(coord.altitude(), 0, 'f', 2);
    }
    writer.writeTextElement("coordinates", coordinates);
    writer.writeEndElement(); // LineString
    writer.writeEndElement(); // Placemark

    // Write waypoint placemarks
    for (int i = 0; i < waypoints.count(); ++i) {
        writer.writeCharacters(generatePlacemark(waypoints[i], i));
    }

    writer.writeEndElement(); // Document
    writer.writeEndElement(); // kml
    writer.writeEndDocument();

    kml = QString::fromUtf8(buffer.data());
    return kml;
}

QString KMZGenerator::generatePlacemark(const Models::Waypoint& waypoint, int index)
{
    QString placemark;
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);

    QXmlStreamWriter writer(&buffer);
    writer.writeStartElement("Placemark");
    writer.writeTextElement("name", QString("WP %1").arg(index + 1));
    writer.writeTextElement("styleUrl", "#waypointStyle");

    writer.writeStartElement("Point");
    writer.writeTextElement("altitudeMode", "relativeToGround");

    const auto& coord = waypoint.coordinate();
    QString coordinates = QString("%1,%2,%3")
        .arg(coord.longitude(), 0, 'f', 8)
        .arg(coord.latitude(), 0, 'f', 8)
        .arg(coord.altitude(), 0, 'f', 2);
    writer.writeTextElement("coordinates", coordinates);

    writer.writeEndElement(); // Point
    writer.writeEndElement(); // Placemark

    placemark = QString::fromUtf8(buffer.data());
    return placemark;
}

} // namespace Geospatial
} // namespace DroneMapper
