#ifndef PROJECTEXPORTER_H
#define PROJECTEXPORTER_H

#include "FlightPlan.h"
#include "MissionParameters.h"
#include <QString>
#include <QJsonObject>
#include <QXmlStreamWriter>
#include <QDateTime>

namespace DroneMapper {
namespace Core {

/**
 * @brief Project export/import formats
 */
enum class ExportFormat {
    JSON,           // JSON format (default)
    XML,            // XML format
    KMZ,            // Google Earth KMZ (waypoints only)
    WPML,           // DJI WPML format (waypoints only)
    GeoJSON,        // GeoJSON format
    CSV,            // CSV waypoint list
    GPX,            // GPS Exchange Format
    Archive         // Compressed project archive (.dmp)
};

/**
 * @brief Project metadata
 */
struct ProjectMetadata {
    QString name;
    QString description;
    QString author;
    QDateTime created;
    QDateTime modified;
    QString version;        // DroneMapper version
    QString formatVersion;  // Project format version
    QStringList tags;

    QJsonObject toJson() const;
    static ProjectMetadata fromJson(const QJsonObject& json);
};

/**
 * @brief Complete project data
 */
struct ProjectData {
    ProjectMetadata metadata;
    Models::FlightPlan flightPlan;
    Models::MissionParameters parameters;
    QByteArray thumbnailImage;  // PNG thumbnail
    QString notes;              // User notes

    QJsonObject toJson() const;
    static ProjectData fromJson(const QJsonObject& json);
};

/**
 * @brief Export/import result
 */
struct ExportResult {
    bool success;
    QString filePath;
    QString errorMessage;
    qint64 fileSizeBytes;
    double exportTimeMs;
};

struct ImportResult {
    bool success;
    ProjectData project;
    QString errorMessage;
    QString warnings;
    double importTimeMs;
};

/**
 * @brief Handles project export and import operations
 *
 * Features:
 * - Multiple export formats (JSON, XML, KMZ, etc.)
 * - Project archiving with compression
 * - Metadata management
 * - Version compatibility checking
 * - Incremental backups
 * - Auto-save functionality
 * - Migration from older formats
 *
 * Supported formats:
 * - JSON (native format with full fidelity)
 * - XML (alternative structured format)
 * - KMZ (Google Earth visualization)
 * - WPML (DJI drone mission format)
 * - GeoJSON (web mapping standard)
 * - CSV (simple waypoint export)
 * - GPX (GPS track format)
 * - Archive (compressed .dmp package)
 */
class ProjectExporter {
public:
    ProjectExporter();

    /**
     * @brief Export project to file
     * @param project Project data to export
     * @param filePath Output file path
     * @param format Export format
     * @return Export result with success status
     */
    static ExportResult exportProject(
        const ProjectData& project,
        const QString& filePath,
        ExportFormat format = ExportFormat::JSON);

    /**
     * @brief Import project from file
     * @param filePath Input file path
     * @return Import result with project data
     */
    static ImportResult importProject(const QString& filePath);

    /**
     * @brief Export flight plan to JSON
     * @param plan Flight plan to export
     * @return JSON object
     */
    static QJsonObject exportFlightPlanJson(const Models::FlightPlan& plan);

    /**
     * @brief Import flight plan from JSON
     * @param json JSON object
     * @return Flight plan
     */
    static Models::FlightPlan importFlightPlanJson(const QJsonObject& json);

    /**
     * @brief Export mission parameters to JSON
     * @param params Parameters to export
     * @return JSON object
     */
    static QJsonObject exportParametersJson(const Models::MissionParameters& params);

    /**
     * @brief Import mission parameters from JSON
     * @param json JSON object
     * @return Mission parameters
     */
    static Models::MissionParameters importParametersJson(const QJsonObject& json);

    /**
     * @brief Create compressed project archive (.dmp)
     * @param project Project data
     * @param filePath Output archive path
     * @return Export result
     */
    static ExportResult createArchive(
        const ProjectData& project,
        const QString& filePath);

    /**
     * @brief Extract project from archive
     * @param filePath Archive file path
     * @return Import result
     */
    static ImportResult extractArchive(const QString& filePath);

    /**
     * @brief Export waypoints to CSV
     * @param plan Flight plan
     * @param filePath Output CSV path
     * @return Export result
     */
    static ExportResult exportToCSV(
        const Models::FlightPlan& plan,
        const QString& filePath);

    /**
     * @brief Export to GeoJSON format
     * @param plan Flight plan
     * @param filePath Output file path
     * @return Export result
     */
    static ExportResult exportToGeoJSON(
        const Models::FlightPlan& plan,
        const QString& filePath);

    /**
     * @brief Export to GPX format
     * @param plan Flight plan
     * @param filePath Output file path
     * @return Export result
     */
    static ExportResult exportToGPX(
        const Models::FlightPlan& plan,
        const QString& filePath);

    /**
     * @brief Check if file format is supported
     * @param filePath File path to check
     * @return True if format is supported
     */
    static bool isSupportedFormat(const QString& filePath);

    /**
     * @brief Detect format from file
     * @param filePath File to analyze
     * @return Detected format
     */
    static ExportFormat detectFormat(const QString& filePath);

    /**
     * @brief Get file extension for format
     * @param format Export format
     * @return File extension (e.g., ".json", ".kmz")
     */
    static QString getFileExtension(ExportFormat format);

    /**
     * @brief Get format name
     * @param format Export format
     * @return Human-readable format name
     */
    static QString getFormatName(ExportFormat format);

    /**
     * @brief Create auto-backup of project
     * @param project Project to backup
     * @param backupDir Backup directory path
     * @return Backup file path (empty on failure)
     */
    static QString createAutoBackup(
        const ProjectData& project,
        const QString& backupDir);

    /**
     * @brief List available backups
     * @param backupDir Backup directory
     * @return List of backup file paths
     */
    static QStringList listBackups(const QString& backupDir);

private:
    static ExportResult exportToJSON(const ProjectData& project, const QString& filePath);
    static ExportResult exportToXML(const ProjectData& project, const QString& filePath);

    static ImportResult importFromJSON(const QString& filePath);
    static ImportResult importFromXML(const QString& filePath);

    static void writeXMLFlightPlan(
        QXmlStreamWriter& xml,
        const Models::FlightPlan& plan);

    static void writeXMLParameters(
        QXmlStreamWriter& xml,
        const Models::MissionParameters& params);

    static bool validateProjectData(const ProjectData& project);
    static QString generateBackupFilename(const QString& projectName);
};

} // namespace Core
} // namespace DroneMapper

#endif // PROJECTEXPORTER_H
