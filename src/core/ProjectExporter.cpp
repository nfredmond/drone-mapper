#include "ProjectExporter.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>

namespace DroneMapper {
namespace Core {

QJsonObject ProjectMetadata::toJson() const
{
    QJsonObject json;
    json["name"] = name;
    json["description"] = description;
    json["author"] = author;
    json["created"] = created.toString(Qt::ISODate);
    json["modified"] = modified.toString(Qt::ISODate);
    json["version"] = version;
    json["formatVersion"] = formatVersion;

    QJsonArray tagsArray;
    for (const auto& tag : tags) {
        tagsArray.append(tag);
    }
    json["tags"] = tagsArray;

    return json;
}

ProjectMetadata ProjectMetadata::fromJson(const QJsonObject& json)
{
    ProjectMetadata meta;
    meta.name = json["name"].toString();
    meta.description = json["description"].toString();
    meta.author = json["author"].toString();
    meta.created = QDateTime::fromString(json["created"].toString(), Qt::ISODate);
    meta.modified = QDateTime::fromString(json["modified"].toString(), Qt::ISODate);
    meta.version = json["version"].toString();
    meta.formatVersion = json["formatVersion"].toString();

    QJsonArray tagsArray = json["tags"].toArray();
    for (const auto& tag : tagsArray) {
        meta.tags.append(tag.toString());
    }

    return meta;
}

QJsonObject ProjectData::toJson() const
{
    QJsonObject json;
    json["metadata"] = metadata.toJson();
    json["flightPlan"] = ProjectExporter::exportFlightPlanJson(flightPlan);
    json["parameters"] = ProjectExporter::exportParametersJson(parameters);
    json["notes"] = notes;

    // Thumbnail as base64
    if (!thumbnailImage.isEmpty()) {
        json["thumbnail"] = QString(thumbnailImage.toBase64());
    }

    return json;
}

ProjectData ProjectData::fromJson(const QJsonObject& json)
{
    ProjectData project;
    project.metadata = ProjectMetadata::fromJson(json["metadata"].toObject());
    project.flightPlan = ProjectExporter::importFlightPlanJson(json["flightPlan"].toObject());
    project.parameters = ProjectExporter::importParametersJson(json["parameters"].toObject());
    project.notes = json["notes"].toString();

    if (json.contains("thumbnail")) {
        project.thumbnailImage = QByteArray::fromBase64(json["thumbnail"].toString().toUtf8());
    }

    return project;
}

ProjectExporter::ProjectExporter()
{
}

ExportResult ProjectExporter::exportProject(
    const ProjectData& project,
    const QString& filePath,
    ExportFormat format)
{
    QElapsedTimer timer;
    timer.start();

    ExportResult result;
    result.filePath = filePath;

    switch (format) {
    case ExportFormat::JSON:
        result = exportToJSON(project, filePath);
        break;

    case ExportFormat::XML:
        result = exportToXML(project, filePath);
        break;

    case ExportFormat::CSV:
        result = exportToCSV(project.flightPlan, filePath);
        break;

    case ExportFormat::GeoJSON:
        result = exportToGeoJSON(project.flightPlan, filePath);
        break;

    case ExportFormat::GPX:
        result = exportToGPX(project.flightPlan, filePath);
        break;

    case ExportFormat::Archive:
        result = createArchive(project, filePath);
        break;

    default:
        result.success = false;
        result.errorMessage = "Unsupported export format";
        break;
    }

    result.exportTimeMs = timer.elapsed();
    return result;
}

ImportResult ProjectExporter::importProject(const QString& filePath)
{
    QElapsedTimer timer;
    timer.start();

    ImportResult result;

    ExportFormat format = detectFormat(filePath);

    switch (format) {
    case ExportFormat::JSON:
        result = importFromJSON(filePath);
        break;

    case ExportFormat::XML:
        result = importFromXML(filePath);
        break;

    case ExportFormat::Archive:
        result = extractArchive(filePath);
        break;

    default:
        result.success = false;
        result.errorMessage = "Unsupported import format";
        break;
    }

    result.importTimeMs = timer.elapsed();
    return result;
}

QJsonObject ProjectExporter::exportFlightPlanJson(const Models::FlightPlan& plan)
{
    QJsonObject json;

    // Export waypoints
    QJsonArray waypointsArray;
    for (const auto& wp : plan.waypoints()) {
        QJsonObject wpJson;
        wpJson["latitude"] = wp.coordinate().latitude();
        wpJson["longitude"] = wp.coordinate().longitude();
        wpJson["altitude"] = wp.coordinate().altitude();
        wpJson["number"] = wp.waypointNumber();
        wpJson["speed"] = wp.speed();
        wpJson["heading"] = wp.heading();
        wpJson["hoverTime"] = wp.hoverTime();

        waypointsArray.append(wpJson);
    }
    json["waypoints"] = waypointsArray;

    json["totalDistance"] = plan.totalDistance();

    return json;
}

Models::FlightPlan ProjectExporter::importFlightPlanJson(const QJsonObject& json)
{
    Models::FlightPlan plan;

    QJsonArray waypointsArray = json["waypoints"].toArray();
    for (const auto& wpValue : waypointsArray) {
        QJsonObject wpJson = wpValue.toObject();

        Models::GeospatialCoordinate coord(
            wpJson["latitude"].toDouble(),
            wpJson["longitude"].toDouble(),
            wpJson["altitude"].toDouble());

        Models::Waypoint wp(coord);
        wp.setWaypointNumber(wpJson["number"].toInt());
        wp.setSpeed(wpJson["speed"].toDouble());
        wp.setHeading(wpJson["heading"].toDouble());
        wp.setHoverTime(wpJson["hoverTime"].toInt());

        plan.addWaypoint(wp);
    }

    return plan;
}

QJsonObject ProjectExporter::exportParametersJson(const Models::MissionParameters& params)
{
    QJsonObject json;

    json["flightAltitude"] = params.flightAltitude();
    json["flightSpeed"] = params.flightSpeed();
    json["pathSpacing"] = params.pathSpacing();
    json["flightDirection"] = params.flightDirection();
    json["reversePath"] = params.reversePath();

    json["cameraModel"] = static_cast<int>(params.cameraModel());
    json["frontOverlap"] = params.frontOverlap();
    json["sideOverlap"] = params.sideOverlap();
    json["cameraAngle"] = params.cameraAngle();

    json["finishAction"] = static_cast<int>(params.finishAction());

    return json;
}

Models::MissionParameters ProjectExporter::importParametersJson(const QJsonObject& json)
{
    Models::MissionParameters params;

    params.setFlightAltitude(json["flightAltitude"].toDouble());
    params.setFlightSpeed(json["flightSpeed"].toDouble());
    params.setPathSpacing(json["pathSpacing"].toDouble());
    params.setFlightDirection(json["flightDirection"].toDouble());
    params.setReversePath(json["reversePath"].toBool());

    params.setCameraModel(
        static_cast<Models::MissionParameters::CameraModel>(json["cameraModel"].toInt()));
    params.setFrontOverlap(json["frontOverlap"].toDouble());
    params.setSideOverlap(json["sideOverlap"].toDouble());
    params.setCameraAngle(json["cameraAngle"].toDouble());

    params.setFinishAction(
        static_cast<Models::MissionParameters::FinishAction>(json["finishAction"].toInt()));

    return params;
}

ExportResult ProjectExporter::createArchive(
    const ProjectData& project,
    const QString& filePath)
{
    // Simplified - just export as JSON with .dmp extension
    return exportToJSON(project, filePath);
}

ImportResult ProjectExporter::extractArchive(const QString& filePath)
{
    // Simplified - just import as JSON
    return importFromJSON(filePath);
}

ExportResult ProjectExporter::exportToCSV(
    const Models::FlightPlan& plan,
    const QString& filePath)
{
    ExportResult result;
    result.filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.success = false;
        result.errorMessage = "Could not open file for writing";
        return result;
    }

    QTextStream out(&file);
    out << "Number,Latitude,Longitude,Altitude,Speed,Heading,HoverTime\n";

    for (const auto& wp : plan.waypoints()) {
        out << wp.waypointNumber() << ","
            << QString::number(wp.coordinate().latitude(), 'f', 8) << ","
            << QString::number(wp.coordinate().longitude(), 'f', 8) << ","
            << QString::number(wp.coordinate().altitude(), 'f', 2) << ","
            << QString::number(wp.speed(), 'f', 2) << ","
            << QString::number(wp.heading(), 'f', 1) << ","
            << wp.hoverTime() << "\n";
    }

    file.close();
    result.success = true;
    result.fileSizeBytes = QFileInfo(filePath).size();

    return result;
}

ExportResult ProjectExporter::exportToGeoJSON(
    const Models::FlightPlan& plan,
    const QString& filePath)
{
    ExportResult result;
    result.filePath = filePath;

    QJsonObject geoJson;
    geoJson["type"] = "FeatureCollection";

    QJsonArray features;
    for (const auto& wp : plan.waypoints()) {
        QJsonObject feature;
        feature["type"] = "Feature";

        QJsonObject geometry;
        geometry["type"] = "Point";

        QJsonArray coordinates;
        coordinates.append(wp.coordinate().longitude());
        coordinates.append(wp.coordinate().latitude());
        coordinates.append(wp.coordinate().altitude());
        geometry["coordinates"] = coordinates;

        feature["geometry"] = geometry;

        QJsonObject properties;
        properties["waypoint"] = wp.waypointNumber();
        properties["altitude"] = wp.coordinate().altitude();
        feature["properties"] = properties;

        features.append(feature);
    }

    geoJson["features"] = features;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.success = false;
        result.errorMessage = "Could not open file for writing";
        return result;
    }

    QJsonDocument doc(geoJson);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    result.success = true;
    result.fileSizeBytes = QFileInfo(filePath).size();

    return result;
}

ExportResult ProjectExporter::exportToGPX(
    const Models::FlightPlan& plan,
    const QString& filePath)
{
    ExportResult result;
    result.filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.success = false;
        result.errorMessage = "Could not open file for writing";
        return result;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeStartElement("gpx");
    xml.writeAttribute("version", "1.1");
    xml.writeAttribute("creator", "DroneMapper");

    xml.writeStartElement("trk");
    xml.writeTextElement("name", "Flight Plan");

    xml.writeStartElement("trkseg");

    for (const auto& wp : plan.waypoints()) {
        xml.writeStartElement("trkpt");
        xml.writeAttribute("lat", QString::number(wp.coordinate().latitude(), 'f', 8));
        xml.writeAttribute("lon", QString::number(wp.coordinate().longitude(), 'f', 8));
        xml.writeTextElement("ele", QString::number(wp.coordinate().altitude(), 'f', 2));
        xml.writeTextElement("name", QString::number(wp.waypointNumber()));
        xml.writeEndElement(); // trkpt
    }

    xml.writeEndElement(); // trkseg
    xml.writeEndElement(); // trk
    xml.writeEndElement(); // gpx
    xml.writeEndDocument();

    file.close();
    result.success = true;
    result.fileSizeBytes = QFileInfo(filePath).size();

    return result;
}

bool ProjectExporter::isSupportedFormat(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    return (ext == "json" || ext == "xml" || ext == "dmp" ||
            ext == "csv" || ext == "geojson" || ext == "gpx");
}

ExportFormat ProjectExporter::detectFormat(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == "json") return ExportFormat::JSON;
    if (ext == "xml") return ExportFormat::XML;
    if (ext == "dmp") return ExportFormat::Archive;
    if (ext == "csv") return ExportFormat::CSV;
    if (ext == "geojson") return ExportFormat::GeoJSON;
    if (ext == "gpx") return ExportFormat::GPX;

    return ExportFormat::JSON; // Default
}

QString ProjectExporter::getFileExtension(ExportFormat format)
{
    switch (format) {
    case ExportFormat::JSON:    return ".json";
    case ExportFormat::XML:     return ".xml";
    case ExportFormat::KMZ:     return ".kmz";
    case ExportFormat::WPML:    return ".wpml";
    case ExportFormat::GeoJSON: return ".geojson";
    case ExportFormat::CSV:     return ".csv";
    case ExportFormat::GPX:     return ".gpx";
    case ExportFormat::Archive: return ".dmp";
    }
    return ".json";
}

QString ProjectExporter::getFormatName(ExportFormat format)
{
    switch (format) {
    case ExportFormat::JSON:    return "JSON";
    case ExportFormat::XML:     return "XML";
    case ExportFormat::KMZ:     return "Google Earth KMZ";
    case ExportFormat::WPML:    return "DJI WPML";
    case ExportFormat::GeoJSON: return "GeoJSON";
    case ExportFormat::CSV:     return "CSV";
    case ExportFormat::GPX:     return "GPX";
    case ExportFormat::Archive: return "DroneMapper Archive";
    }
    return "JSON";
}

QString ProjectExporter::createAutoBackup(
    const ProjectData& project,
    const QString& backupDir)
{
    QString filename = generateBackupFilename(project.metadata.name);
    QString filePath = QDir(backupDir).filePath(filename);

    ExportResult result = exportToJSON(project, filePath);

    return result.success ? filePath : QString();
}

QStringList ProjectExporter::listBackups(const QString& backupDir)
{
    QDir dir(backupDir);
    QStringList filters;
    filters << "backup_*.json" << "backup_*.dmp";

    return dir.entryList(filters, QDir::Files, QDir::Time);
}

ExportResult ProjectExporter::exportToJSON(
    const ProjectData& project,
    const QString& filePath)
{
    ExportResult result;
    result.filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.success = false;
        result.errorMessage = "Could not open file for writing";
        return result;
    }

    QJsonDocument doc(project.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    result.success = true;
    result.fileSizeBytes = QFileInfo(filePath).size();

    return result;
}

ExportResult ProjectExporter::exportToXML(
    const ProjectData& project,
    const QString& filePath)
{
    ExportResult result;
    result.filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        result.success = false;
        result.errorMessage = "Could not open file for writing";
        return result;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeStartElement("DroneMapperProject");

    // Metadata
    xml.writeStartElement("Metadata");
    xml.writeTextElement("Name", project.metadata.name);
    xml.writeTextElement("Description", project.metadata.description);
    xml.writeTextElement("Author", project.metadata.author);
    xml.writeTextElement("Created", project.metadata.created.toString(Qt::ISODate));
    xml.writeTextElement("Modified", project.metadata.modified.toString(Qt::ISODate));
    xml.writeEndElement(); // Metadata

    // Flight plan and parameters
    writeXMLFlightPlan(xml, project.flightPlan);
    writeXMLParameters(xml, project.parameters);

    xml.writeEndElement(); // DroneMapperProject
    xml.writeEndDocument();

    file.close();
    result.success = true;
    result.fileSizeBytes = QFileInfo(filePath).size();

    return result;
}

ImportResult ProjectExporter::importFromJSON(const QString& filePath)
{
    ImportResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.success = false;
        result.errorMessage = "Could not open file for reading";
        return result;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        result.success = false;
        result.errorMessage = "Invalid JSON format";
        return result;
    }

    result.project = ProjectData::fromJson(doc.object());
    result.success = true;

    return result;
}

ImportResult ProjectExporter::importFromXML(const QString& filePath)
{
    ImportResult result;
    result.success = false;
    result.errorMessage = "XML import not fully implemented";
    return result;
}

void ProjectExporter::writeXMLFlightPlan(
    QXmlStreamWriter& xml,
    const Models::FlightPlan& plan)
{
    xml.writeStartElement("FlightPlan");

    for (const auto& wp : plan.waypoints()) {
        xml.writeStartElement("Waypoint");
        xml.writeAttribute("number", QString::number(wp.waypointNumber()));
        xml.writeTextElement("Latitude", QString::number(wp.coordinate().latitude(), 'f', 8));
        xml.writeTextElement("Longitude", QString::number(wp.coordinate().longitude(), 'f', 8));
        xml.writeTextElement("Altitude", QString::number(wp.coordinate().altitude(), 'f', 2));
        xml.writeTextElement("Speed", QString::number(wp.speed(), 'f', 2));
        xml.writeTextElement("Heading", QString::number(wp.heading(), 'f', 1));
        xml.writeEndElement(); // Waypoint
    }

    xml.writeEndElement(); // FlightPlan
}

void ProjectExporter::writeXMLParameters(
    QXmlStreamWriter& xml,
    const Models::MissionParameters& params)
{
    xml.writeStartElement("Parameters");
    xml.writeTextElement("FlightAltitude", QString::number(params.flightAltitude()));
    xml.writeTextElement("FlightSpeed", QString::number(params.flightSpeed()));
    xml.writeTextElement("FrontOverlap", QString::number(params.frontOverlap()));
    xml.writeTextElement("SideOverlap", QString::number(params.sideOverlap()));
    xml.writeEndElement(); // Parameters
}

bool ProjectExporter::validateProjectData(const ProjectData& project)
{
    if (project.metadata.name.isEmpty()) return false;
    if (project.flightPlan.waypoints().isEmpty()) return false;
    return true;
}

QString ProjectExporter::generateBackupFilename(const QString& projectName)
{
    QString safeName = projectName;
    safeName.replace(" ", "_");
    safeName.replace("/", "_");

    QDateTime now = QDateTime::currentDateTime();
    return QString("backup_%1_%2.json")
        .arg(safeName)
        .arg(now.toString("yyyyMMdd_HHmmss"));
}

} // namespace Core
} // namespace DroneMapper
