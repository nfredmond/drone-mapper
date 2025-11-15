#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QString>
#include <QImage>
#include <QDateTime>
#include "models/FlightPlan.h"
#include "models/MissionParameters.h"

namespace DroneMapper {
namespace Core {

/**
 * @brief Report format types
 */
enum class ReportFormat {
    PDF,        // PDF document
    HTML,       // HTML document
    Markdown,   // Markdown format
    DOCX        // Microsoft Word (future)
};

/**
 * @brief Report sections to include
 */
struct ReportSections {
    bool includeCoverPage;
    bool includeMissionOverview;
    bool includeFlightPathMap;
    bool includeStatistics;
    bool includeCostBreakdown;
    bool includeWeatherAnalysis;
    bool includeSafetyAnalysis;
    bool includeEquipment;
    bool includePhotogrammetryPlan;
    bool includeAppendices;

    ReportSections();
};

/**
 * @brief Report customization options
 */
struct ReportOptions {
    QString companyName;
    QString companyLogo;  // Path to logo image
    QString projectName;
    QString clientName;
    QString pilotName;
    QString droneRegistration;

    bool includeConfidentialWatermark;
    bool includeDraftWatermark;

    QString additionalNotes;
    QString customFooter;

    ReportSections sections;

    ReportOptions();
};

/**
 * @brief Automated Report Generator
 *
 * Generates professional mission reports in PDF/HTML format including:
 * - Cover page with project info
 * - Mission overview and objectives
 * - Flight path visualization with maps
 * - Comprehensive statistics (distance, time, photos, area)
 * - Cost breakdown (battery, time, processing)
 * - Weather conditions and flight suitability
 * - Safety analysis and risk assessment
 * - Equipment checklist
 * - Photogrammetry quality predictions
 * - Regulatory compliance notes
 *
 * Usage:
 *   ReportGenerator generator;
 *   ReportOptions options;
 *   options.companyName = "Acme Drones Inc.";
 *   options.projectName = "Site Survey 2025";
 *
 *   QString outputPath = "/path/to/report.pdf";
 *   if (generator.generateReport(flightPlan, outputPath, ReportFormat::PDF, options)) {
 *       qDebug() << "Report generated successfully!";
 *   }
 */
class ReportGenerator {
public:
    ReportGenerator();
    ~ReportGenerator();

    /**
     * @brief Generate mission report
     * @param plan Flight plan
     * @param outputPath Output file path
     * @param format Report format
     * @param options Report options
     * @return True if successful
     */
    bool generateReport(
        const Models::FlightPlan& plan,
        const QString& outputPath,
        ReportFormat format,
        const ReportOptions& options);

    /**
     * @brief Generate quick summary report (minimal)
     * @param plan Flight plan
     * @param outputPath Output file path
     * @return True if successful
     */
    bool generateQuickSummary(
        const Models::FlightPlan& plan,
        const QString& outputPath);

    /**
     * @brief Get last error message
     * @return Error message
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;

    // PDF generation
    bool generatePDF(
        const Models::FlightPlan& plan,
        const QString& outputPath,
        const ReportOptions& options);

    // HTML generation
    bool generateHTML(
        const Models::FlightPlan& plan,
        const QString& outputPath,
        const ReportOptions& options);

    // Markdown generation
    bool generateMarkdown(
        const Models::FlightPlan& plan,
        const QString& outputPath,
        const ReportOptions& options);

    // Section generators
    QString generateCoverPage(const ReportOptions& options);
    QString generateMissionOverview(const Models::FlightPlan& plan, const ReportOptions& options);
    QString generateStatistics(const Models::FlightPlan& plan);
    QString generateCostBreakdown(const Models::FlightPlan& plan);
    QString generateWeatherAnalysis(const Models::FlightPlan& plan);
    QString generateSafetyAnalysis(const Models::FlightPlan& plan);
    QString generateEquipmentList(const Models::FlightPlan& plan);
    QString generatePhotogrammetryPlan(const Models::FlightPlan& plan);

    // Helper methods
    QImage generateMapImage(const Models::FlightPlan& plan, int width, int height);
    QImage generateFlightPathDiagram(const Models::FlightPlan& plan);
    QString formatDateTime(const QDateTime& dt);
    QString formatDuration(int seconds);
    QString formatDistance(double meters);
    QString formatArea(double squareMeters);

    // HTML template methods
    QString getHTMLTemplate();
    QString getCSSStyle();

    // Statistics calculation
    struct ReportStatistics {
        double totalDistance;
        int estimatedFlightTime;
        int estimatedPhotoCount;
        double surveyArea;
        double gsd;
        int batteryCount;
        double totalCost;
    };

    ReportStatistics calculateStatistics(const Models::FlightPlan& plan);
};

} // namespace Core
} // namespace DroneMapper

#endif // REPORTGENERATOR_H
