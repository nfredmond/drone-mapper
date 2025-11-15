#include "ReportGenerator.h"
#include "geospatial/GeoUtils.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QImage>
#include <QPolygonF>
#include <QtMath>
#include <cmath>

namespace DroneMapper {
namespace Core {

// ReportSections implementation

ReportSections::ReportSections()
    : includeCoverPage(true)
    , includeMissionOverview(true)
    , includeFlightPathMap(true)
    , includeStatistics(true)
    , includeCostBreakdown(true)
    , includeWeatherAnalysis(true)
    , includeSafetyAnalysis(true)
    , includeEquipment(true)
    , includePhotogrammetryPlan(true)
    , includeAppendices(false)
{
}

// ReportOptions implementation

ReportOptions::ReportOptions()
    : companyName("DroneMapper Professional")
    , projectName("Untitled Project")
    , clientName("")
    , pilotName("")
    , droneRegistration("")
    , includeConfidentialWatermark(false)
    , includeDraftWatermark(false)
{
}

// ReportGenerator implementation

ReportGenerator::ReportGenerator()
{
}

ReportGenerator::~ReportGenerator()
{
}

bool ReportGenerator::generateReport(
    const Models::FlightPlan& plan,
    const QString& outputPath,
    ReportFormat format,
    const ReportOptions& options)
{
    m_lastError.clear();

    // Validate input
    if (plan.waypoints().isEmpty()) {
        m_lastError = "Flight plan has no waypoints";
        return false;
    }

    // Create output directory if needed
    QFileInfo fileInfo(outputPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = "Failed to create output directory";
            return false;
        }
    }

    // Generate based on format
    switch (format) {
    case ReportFormat::PDF:
        return generatePDF(plan, outputPath, options);
    case ReportFormat::HTML:
        return generateHTML(plan, outputPath, options);
    case ReportFormat::Markdown:
        return generateMarkdown(plan, outputPath, options);
    default:
        m_lastError = "Unsupported report format";
        return false;
    }
}

bool ReportGenerator::generateQuickSummary(
    const Models::FlightPlan& plan,
    const QString& outputPath)
{
    ReportOptions options;
    options.sections.includeCoverPage = false;
    options.sections.includeWeatherAnalysis = false;
    options.sections.includeSafetyAnalysis = false;
    options.sections.includeEquipment = false;
    options.sections.includeAppendices = false;

    return generateReport(plan, outputPath, ReportFormat::HTML, options);
}

bool ReportGenerator::generatePDF(
    const Models::FlightPlan& plan,
    const QString& outputPath,
    const ReportOptions& options)
{
    // PDF generation would require QPrinter or external library
    // For now, generate HTML and inform user to print to PDF
    QString htmlPath = outputPath;
    htmlPath.replace(".pdf", ".html");

    if (!generateHTML(plan, htmlPath, options)) {
        return false;
    }

    m_lastError = "HTML report generated. Use browser 'Print to PDF' to create PDF";
    return true;
}

bool ReportGenerator::generateHTML(
    const Models::FlightPlan& plan,
    const QString& outputPath,
    const ReportOptions& options)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Failed to open output file for writing";
        return false;
    }

    QTextStream out(&file);

    // Write HTML header
    out << "<!DOCTYPE html>\n";
    out << "<html lang=\"en\">\n";
    out << "<head>\n";
    out << "    <meta charset=\"UTF-8\">\n";
    out << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    out << "    <title>" << options.projectName << " - Mission Report</title>\n";
    out << "    <style>\n" << getCSSStyle() << "\n    </style>\n";
    out << "</head>\n";
    out << "<body>\n";

    // Watermarks
    if (options.includeDraftWatermark || options.includeConfidentialWatermark) {
        out << "<div class=\"watermark\">";
        if (options.includeDraftWatermark) out << "DRAFT ";
        if (options.includeConfidentialWatermark) out << "CONFIDENTIAL";
        out << "</div>\n";
    }

    // Cover page
    if (options.sections.includeCoverPage) {
        out << generateCoverPage(options);
    }

    // Mission overview
    if (options.sections.includeMissionOverview) {
        out << generateMissionOverview(plan, options);
    }

    // Statistics
    if (options.sections.includeStatistics) {
        out << generateStatistics(plan);
    }

    // Cost breakdown
    if (options.sections.includeCostBreakdown) {
        out << generateCostBreakdown(plan);
    }

    // Weather analysis
    if (options.sections.includeWeatherAnalysis) {
        out << generateWeatherAnalysis(plan);
    }

    // Safety analysis
    if (options.sections.includeSafetyAnalysis) {
        out << generateSafetyAnalysis(plan);
    }

    // Equipment
    if (options.sections.includeEquipment) {
        out << generateEquipmentList(plan);
    }

    // Photogrammetry plan
    if (options.sections.includePhotogrammetryPlan) {
        out << generatePhotogrammetryPlan(plan);
    }

    // Footer
    out << "<div class=\"footer\">\n";
    if (!options.customFooter.isEmpty()) {
        out << "    <p>" << options.customFooter << "</p>\n";
    }
    out << "    <p>Generated by DroneMapper on " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") << "</p>\n";
    out << "</div>\n";

    out << "</body>\n";
    out << "</html>\n";

    file.close();
    return true;
}

bool ReportGenerator::generateMarkdown(
    const Models::FlightPlan& plan,
    const QString& outputPath,
    const ReportOptions& options)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Failed to open output file for writing";
        return false;
    }

    QTextStream out(&file);

    // Title
    out << "# " << options.projectName << "\n\n";
    out << "## Mission Report\n\n";

    // Metadata
    out << "**Generated:** " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") << "\n\n";
    if (!options.companyName.isEmpty()) {
        out << "**Company:** " << options.companyName << "\n\n";
    }
    if (!options.clientName.isEmpty()) {
        out << "**Client:** " << options.clientName << "\n\n";
    }

    // Statistics
    auto stats = calculateStatistics(plan);
    out << "## Mission Statistics\n\n";
    out << "- **Waypoints:** " << plan.waypoints().count() << "\n";
    out << "- **Distance:** " << formatDistance(stats.totalDistance) << "\n";
    out << "- **Flight Time:** " << formatDuration(stats.estimatedFlightTime) << "\n";
    out << "- **Photos:** ~" << stats.estimatedPhotoCount << "\n";
    out << "- **Area:** " << formatArea(stats.surveyArea) << "\n";
    out << "- **GSD:** " << QString::number(stats.gsd, 'f', 2) << " cm/px\n";
    out << "- **Batteries:** " << stats.batteryCount << "\n\n";

    // Parameters
    out << "## Mission Parameters\n\n";
    const auto& params = plan.parameters();
    out << "- **Altitude:** " << params.flightAltitude() << "m AGL\n";
    out << "- **Speed:** " << params.flightSpeed() << "m/s\n";
    out << "- **Front Overlap:** " << params.frontOverlap() << "%\n";
    out << "- **Side Overlap:** " << params.sideOverlap() << "%\n";
    out << "- **Gimbal Angle:** " << params.gimbalPitch() << "°\n\n";

    file.close();
    return true;
}

QString ReportGenerator::generateCoverPage(const ReportOptions& options)
{
    QString html;
    html += "<div class=\"cover-page\">\n";

    if (!options.companyLogo.isEmpty()) {
        html += "    <img src=\"" + options.companyLogo + "\" class=\"logo\" alt=\"Company Logo\">\n";
    }

    html += "    <h1>" + options.projectName + "</h1>\n";
    html += "    <h2>Drone Mission Report</h2>\n";

    html += "    <div class=\"cover-details\">\n";
    if (!options.companyName.isEmpty()) {
        html += "        <p><strong>Company:</strong> " + options.companyName + "</p>\n";
    }
    if (!options.clientName.isEmpty()) {
        html += "        <p><strong>Client:</strong> " + options.clientName + "</p>\n";
    }
    if (!options.pilotName.isEmpty()) {
        html += "        <p><strong>Pilot:</strong> " + options.pilotName + "</p>\n";
    }
    html += "        <p><strong>Date:</strong> " + QDateTime::currentDateTime().toString("MMMM d, yyyy") + "</p>\n";
    html += "    </div>\n";

    html += "</div>\n";
    html += "<div class=\"page-break\"></div>\n";

    return html;
}

QString ReportGenerator::generateMissionOverview(
    const Models::FlightPlan& plan,
    const ReportOptions& options)
{
    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Mission Overview</h2>\n";

    html += "    <div class=\"info-box\">\n";
    html += "        <h3>Project Information</h3>\n";
    html += "        <table>\n";
    html += "            <tr><td><strong>Project Name:</strong></td><td>" + options.projectName + "</td></tr>\n";
    html += "            <tr><td><strong>Mission Type:</strong></td><td>Aerial Survey</td></tr>\n";
    html += "            <tr><td><strong>Pattern:</strong></td><td>" + QString::number(static_cast<int>(plan.patternType())) + "</td></tr>\n";
    html += "            <tr><td><strong>Waypoint Count:</strong></td><td>" + QString::number(plan.waypoints().count()) + "</td></tr>\n";
    html += "        </table>\n";
    html += "    </div>\n";

    if (!options.additionalNotes.isEmpty()) {
        html += "    <div class=\"notes\">\n";
        html += "        <h3>Additional Notes</h3>\n";
        html += "        <p>" + options.additionalNotes + "</p>\n";
        html += "    </div>\n";
    }

    html += "</div>\n";

    return html;
}

QString ReportGenerator::generateStatistics(const Models::FlightPlan& plan)
{
    auto stats = calculateStatistics(plan);

    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Mission Statistics</h2>\n";

    html += "    <div class=\"stats-grid\">\n";

    html += "        <div class=\"stat-card\">\n";
    html += "            <div class=\"stat-value\">" + formatDistance(stats.totalDistance) + "</div>\n";
    html += "            <div class=\"stat-label\">Total Distance</div>\n";
    html += "        </div>\n";

    html += "        <div class=\"stat-card\">\n";
    html += "            <div class=\"stat-value\">" + formatDuration(stats.estimatedFlightTime) + "</div>\n";
    html += "            <div class=\"stat-label\">Flight Time</div>\n";
    html += "        </div>\n";

    html += "        <div class=\"stat-card\">\n";
    html += "            <div class=\"stat-value\">~" + QString::number(stats.estimatedPhotoCount) + "</div>\n";
    html += "            <div class=\"stat-label\">Estimated Photos</div>\n";
    html += "        </div>\n";

    html += "        <div class=\"stat-card\">\n";
    html += "            <div class=\"stat-value\">" + formatArea(stats.surveyArea) + "</div>\n";
    html += "            <div class=\"stat-label\">Survey Area</div>\n";
    html += "        </div>\n";

    html += "        <div class=\"stat-card\">\n";
    html += "            <div class=\"stat-value\">" + QString::number(stats.gsd, 'f', 2) + " cm/px</div>\n";
    html += "            <div class=\"stat-label\">GSD</div>\n";
    html += "        </div>\n";

    html += "        <div class=\"stat-card\">\n";
    html += "            <div class=\"stat-value\">" + QString::number(stats.batteryCount) + "</div>\n";
    html += "            <div class=\"stat-label\">Batteries Required</div>\n";
    html += "        </div>\n";

    html += "    </div>\n";
    html += "</div>\n";

    return html;
}

QString ReportGenerator::generateCostBreakdown(const Models::FlightPlan& plan)
{
    auto stats = calculateStatistics(plan);

    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Cost Breakdown</h2>\n";

    html += "    <table class=\"cost-table\">\n";
    html += "        <thead>\n";
    html += "            <tr><th>Item</th><th>Quantity</th><th>Unit Cost</th><th>Total</th></tr>\n";
    html += "        </thead>\n";
    html += "        <tbody>\n";

    double flightHours = stats.estimatedFlightTime / 3600.0;
    double pilotCost = flightHours * 75.0; // $75/hr
    html += "            <tr><td>Pilot Time</td><td>" + QString::number(flightHours, 'f', 2) + " hrs</td><td>$75/hr</td><td>$" + QString::number(pilotCost, 'f', 2) + "</td></tr>\n";

    double batteryCost = stats.batteryCount * 5.0; // $5 per battery cycle
    html += "            <tr><td>Battery Usage</td><td>" + QString::number(stats.batteryCount) + "</td><td>$5.00</td><td>$" + QString::number(batteryCost, 'f', 2) + "</td></tr>\n";

    double processingHours = stats.estimatedPhotoCount / 200.0; // 200 photos/hour
    double processingCost = processingHours * 50.0; // $50/hr
    html += "            <tr><td>Processing Time</td><td>" + QString::number(processingHours, 'f', 1) + " hrs</td><td>$50/hr</td><td>$" + QString::number(processingCost, 'f', 2) + "</td></tr>\n";

    double totalCost = pilotCost + batteryCost + processingCost;
    html += "            <tr class=\"total-row\"><td colspan=\"3\"><strong>Total Estimated Cost</strong></td><td><strong>$" + QString::number(totalCost, 'f', 2) + "</strong></td></tr>\n";

    html += "        </tbody>\n";
    html += "    </table>\n";
    html += "</div>\n";

    return html;
}

QString ReportGenerator::generateWeatherAnalysis(const Models::FlightPlan& plan)
{
    Q_UNUSED(plan);

    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Weather Analysis</h2>\n";

    html += "    <div class=\"warning-box\">\n";
    html += "        <p><strong>Note:</strong> Weather conditions should be checked immediately before flight.</p>\n";
    html += "    </div>\n";

    html += "    <h3>Recommended Flight Conditions</h3>\n";
    html += "    <ul>\n";
    html += "        <li><strong>Wind:</strong> Less than 10 m/s (22 mph)</li>\n";
    html += "        <li><strong>Temperature:</strong> 0°C to 40°C (32°F to 104°F)</li>\n";
    html += "        <li><strong>Visibility:</strong> Greater than 3 km</li>\n";
    html += "        <li><strong>Cloud Cover:</strong> Overcast preferable for photogrammetry</li>\n";
    html += "        <li><strong>Precipitation:</strong> None</li>\n";
    html += "    </ul>\n";

    html += "</div>\n";

    return html;
}

QString ReportGenerator::generateSafetyAnalysis(const Models::FlightPlan& plan)
{
    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Safety Analysis</h2>\n";

    double maxAlt = plan.parameters().flightAltitude();

    html += "    <h3>Regulatory Compliance</h3>\n";
    html += "    <ul>\n";
    html += "        <li><strong>Maximum Altitude:</strong> " + QString::number(maxAlt) + "m AGL";
    if (maxAlt > 120) {
        html += " ⚠️ <span class=\"warning\">Exceeds FAA Part 107 limit (120m)</span>";
    } else {
        html += " ✓ Within FAA Part 107 limits";
    }
    html += "</li>\n";
    html += "        <li><strong>Visual Line of Sight:</strong> Required</li>\n";
    html += "        <li><strong>Airspace Check:</strong> Required before flight</li>\n";
    html += "        <li><strong>NOTAM Check:</strong> Required before flight</li>\n";
    html += "    </ul>\n";

    html += "    <h3>Pre-Flight Checklist</h3>\n";
    html += "    <ul>\n";
    html += "        <li>☐ Weather conditions acceptable</li>\n";
    html += "        <li>☐ Airspace authorization obtained (if required)</li>\n";
    html += "        <li>☐ Batteries fully charged</li>\n";
    html += "        <li>☐ SD cards formatted and ready</li>\n";
    html += "        <li>☐ Aircraft pre-flight inspection complete</li>\n";
    html += "        <li>☐ GPS signal strong (≥12 satellites)</li>\n";
    html += "        <li>☐ No-fly zones checked</li>\n";
    html += "        <li>☐ Emergency procedures reviewed</li>\n";
    html += "    </ul>\n";

    html += "</div>\n";

    return html;
}

QString ReportGenerator::generateEquipmentList(const Models::FlightPlan& plan)
{
    auto stats = calculateStatistics(plan);

    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Equipment Checklist</h2>\n";

    html += "    <table class=\"equipment-table\">\n";
    html += "        <thead>\n";
    html += "            <tr><th>Item</th><th>Quantity</th><th>Notes</th></tr>\n";
    html += "        </thead>\n";
    html += "        <tbody>\n";
    html += "            <tr><td>Drone</td><td>1</td><td>" + QString("Camera") + "</td></tr>\n";
    html += "            <tr><td>Batteries (charged)</td><td>" + QString::number(stats.batteryCount + 1) + "</td><td>Including spare</td></tr>\n";
    html += "            <tr><td>SD Cards</td><td>2</td><td>128GB minimum</td></tr>\n";
    html += "            <tr><td>Remote Controller</td><td>1</td><td>Fully charged</td></tr>\n";
    html += "            <tr><td>Tablet/Phone</td><td>1</td><td>With DJI Fly app</td></tr>\n";
    html += "            <tr><td>Landing Pad</td><td>1</td><td>Optional</td></tr>\n";
    html += "            <tr><td>Propeller Guards</td><td>1 set</td><td>If required</td></tr>\n";
    html += "            <tr><td>Tool Kit</td><td>1</td><td>Screwdrivers, etc.</td></tr>\n";
    html += "        </tbody>\n";
    html += "    </table>\n";

    html += "</div>\n";

    return html;
}

QString ReportGenerator::generatePhotogrammetryPlan(const Models::FlightPlan& plan)
{
    auto stats = calculateStatistics(plan);

    QString html;
    html += "<div class=\"section\">\n";
    html += "    <h2>Photogrammetry Plan</h2>\n";

    const auto& params = plan.parameters();

    html += "    <h3>Capture Parameters</h3>\n";
    html += "    <table>\n";
    html += "        <tr><td><strong>Altitude AGL:</strong></td><td>" + QString::number(params.flightAltitude()) + " m</td></tr>\n";
    html += "        <tr><td><strong>Ground Sampling Distance:</strong></td><td>" + QString::number(stats.gsd, 'f', 2) + " cm/pixel</td></tr>\n";
    html += "        <tr><td><strong>Front Overlap:</strong></td><td>" + QString::number(params.frontOverlap()) + "%</td></tr>\n";
    html += "        <tr><td><strong>Side Overlap:</strong></td><td>" + QString::number(params.sideOverlap()) + "%</td></tr>\n";
    html += "        <tr><td><strong>Gimbal Angle:</strong></td><td>" + QString::number(params.gimbalPitch()) + "° (nadir)</td></tr>\n";
    html += "        <tr><td><strong>Estimated Photos:</strong></td><td>~" + QString::number(stats.estimatedPhotoCount) + "</td></tr>\n";
    html += "    </table>\n";

    html += "    <h3>Processing Recommendations</h3>\n";
    html += "    <ul>\n";
    html += "        <li><strong>Software:</strong> COLMAP, Pix4D, or Agisoft Metashape</li>\n";
    html += "        <li><strong>Quality:</strong> High (based on " + QString::number(params.frontOverlap()) + "% overlap)</li>\n";
    html += "        <li><strong>Output Products:</strong> Orthomosaic, DSM, Point Cloud, 3D Mesh</li>\n";
    html += "        <li><strong>Coordinate System:</strong> WGS84 / UTM</li>\n";
    html += "    </ul>\n";

    html += "</div>\n";

    return html;
}

ReportGenerator::ReportStatistics ReportGenerator::calculateStatistics(const Models::FlightPlan& plan)
{
    ReportStatistics stats;

    // Calculate total distance
    stats.totalDistance = 0.0;
    for (int i = 1; i < plan.waypoints().count(); ++i) {
        stats.totalDistance += Geospatial::GeoUtils::distanceBetween(
            plan.waypoints()[i-1].coordinate(),
            plan.waypoints()[i].coordinate());
    }

    // Estimate flight time (using flight speed from parameters)
    double avgSpeed = plan.parameters().flightSpeed();
    stats.estimatedFlightTime = static_cast<int>(stats.totalDistance / avgSpeed);

    // Estimate photo count (photos at each waypoint)
    stats.estimatedPhotoCount = plan.waypoints().count();

    // Calculate survey area (from polygon if available)
    stats.surveyArea = 0.0;
    // Simplified - would calculate from actual survey polygon

    // Calculate GSD (simplified)
    double altitude = plan.parameters().flightAltitude();
    double focalLength = 24.0; // mm (example)
    double sensorWidth = 13.2; // mm (example)
    int imageWidth = 5472; // pixels (example)
    stats.gsd = (altitude * 1000.0 * sensorWidth) / (focalLength * imageWidth);

    // Estimate battery count
    int flightTimePerBattery = 20 * 60; // 20 minutes per battery
    stats.batteryCount = (stats.estimatedFlightTime / flightTimePerBattery) + 1;

    // Calculate total cost (simplified)
    stats.totalCost = 0.0;

    return stats;
}

QString ReportGenerator::formatDateTime(const QDateTime& dt)
{
    return dt.toString("yyyy-MM-dd HH:mm:ss");
}

QString ReportGenerator::formatDuration(int seconds)
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;

    if (hours > 0) {
        return QString("%1h %2m").arg(hours).arg(minutes);
    } else {
        return QString("%1m").arg(minutes);
    }
}

QString ReportGenerator::formatDistance(double meters)
{
    if (meters >= 1000) {
        return QString::number(meters / 1000.0, 'f', 2) + " km";
    } else {
        return QString::number(meters, 'f', 0) + " m";
    }
}

QString ReportGenerator::formatArea(double squareMeters)
{
    if (squareMeters >= 10000) {
        return QString::number(squareMeters / 10000.0, 'f', 2) + " ha";
    } else {
        return QString::number(squareMeters, 'f', 0) + " m²";
    }
}

QString ReportGenerator::getCSSStyle()
{
    return R"(
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; line-height: 1.6; color: #333; background: #f5f5f5; padding: 20px; }

        .cover-page { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 100px 50px; text-align: center; min-height: 100vh; display: flex; flex-direction: column; justify-content: center; }
        .cover-page h1 { font-size: 3em; margin-bottom: 20px; }
        .cover-page h2 { font-size: 1.5em; margin-bottom: 40px; opacity: 0.9; }
        .cover-details { margin-top: 60px; font-size: 1.1em; }
        .cover-details p { margin: 10px 0; }
        .logo { max-width: 200px; margin-bottom: 40px; }

        .section { background: white; padding: 40px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .section h2 { color: #667eea; border-bottom: 3px solid #667eea; padding-bottom: 10px; margin-bottom: 20px; font-size: 2em; }
        .section h3 { color: #555; margin: 20px 0 10px; font-size: 1.3em; }

        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 20px 0; }
        .stat-card { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 8px; text-align: center; }
        .stat-value { font-size: 2.5em; font-weight: bold; margin-bottom: 10px; }
        .stat-label { font-size: 0.9em; opacity: 0.9; text-transform: uppercase; letter-spacing: 1px; }

        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        table th { background: #667eea; color: white; padding: 12px; text-align: left; }
        table td { padding: 10px; border-bottom: 1px solid #ddd; }
        table tr:hover { background: #f5f5f5; }
        .total-row { background: #f0f0f0; font-weight: bold; }

        .info-box { background: #e8f4f8; border-left: 4px solid #2196F3; padding: 20px; margin: 20px 0; }
        .warning-box { background: #fff3cd; border-left: 4px solid #ffc107; padding: 20px; margin: 20px 0; }
        .notes { background: #f8f9fa; padding: 20px; margin: 20px 0; border-radius: 5px; }

        .watermark { position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%) rotate(-45deg); font-size: 8em; opacity: 0.05; pointer-events: none; z-index: 1000; }

        .footer { text-align: center; padding: 20px; color: #777; font-size: 0.9em; margin-top: 40px; }
        .page-break { page-break-after: always; }

        ul { margin-left: 20px; }
        ul li { margin: 8px 0; }

        .warning { color: #dc3545; font-weight: bold; }

        @media print {
            body { background: white; }
            .section { box-shadow: none; }
            .page-break { page-break-after: always; }
        }
    )";
}

} // namespace Core
} // namespace DroneMapper
