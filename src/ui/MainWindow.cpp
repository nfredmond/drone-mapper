#include "MainWindow.h"
#include "MapWidget.h"
#include "MissionParametersDialog.h"
#include "Settings.h"
#include "ProjectManager.h"
#include "MissionParameters.h"
#include "FlightPlan.h"
#include "CoveragePatternGenerator.h"
#include "KMZGenerator.h"
#include "WPMLWriter.h"
#include "GeoUtils.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>
#include <cmath>

namespace DroneMapper {
namespace UI {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mapWidget(new MapWidget(this))
    , m_currentFlightPlan(nullptr)
{
    setWindowTitle("DroneMapper - Flight Planning & Photogrammetry");
    resize(1280, 800);

    // Set map widget as central widget
    setCentralWidget(m_mapWidget);

    // Connect map widget signals
    connect(m_mapWidget, &MapWidget::areaSelected, this, &MainWindow::onAreaSelected);
    connect(m_mapWidget, &MapWidget::flightPlanRequested, this, &MainWindow::onFlightPlanRequested);

    createActions();
    createMenus();
    createToolbars();
    createDockWidgets();
    readSettings();

    // Set default map center (can be changed based on user settings)
    m_mapWidget->setCenter(37.7749, -122.4194, 12); // San Francisco default

    statusBar()->showMessage("Ready - Draw an area on the map to begin flight planning", 10000);
}

MainWindow::~MainWindow()
{
    writeSettings();
}

void MainWindow::createActions()
{
    m_newProjectAction = new QAction(tr("&New Project..."), this);
    m_newProjectAction->setShortcut(QKeySequence::New);
    connect(m_newProjectAction, &QAction::triggered, this, &MainWindow::newProject);

    m_openProjectAction = new QAction(tr("&Open Project..."), this);
    m_openProjectAction->setShortcut(QKeySequence::Open);
    connect(m_openProjectAction, &QAction::triggered, this, &MainWindow::openProject);

    m_saveProjectAction = new QAction(tr("&Save Project"), this);
    m_saveProjectAction->setShortcut(QKeySequence::Save);
    connect(m_saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);

    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::exit);

    // Map and flight planning actions
    m_generateFlightPlanAction = new QAction(tr("&Generate Flight Plan"), this);
    m_generateFlightPlanAction->setShortcut(QKeySequence(tr("Ctrl+G")));
    m_generateFlightPlanAction->setEnabled(false); // Enabled when area is selected
    connect(m_generateFlightPlanAction, &QAction::triggered, this, &MainWindow::onGenerateFlightPlan);

    m_exportKMZAction = new QAction(tr("&Export KMZ..."), this);
    m_exportKMZAction->setShortcut(QKeySequence(tr("Ctrl+E")));
    m_exportKMZAction->setEnabled(false); // Enabled when flight plan exists
    connect(m_exportKMZAction, &QAction::triggered, this, &MainWindow::onExportKMZ);

    m_clearMapAction = new QAction(tr("&Clear Map"), this);
    m_clearMapAction->setShortcut(QKeySequence(tr("Ctrl+K")));
    connect(m_clearMapAction, &QAction::triggered, this, &MainWindow::onClearMap);
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_newProjectAction);
    m_fileMenu->addAction(m_openProjectAction);
    m_fileMenu->addAction(m_saveProjectAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exportKMZAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));

    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_toolsMenu->addAction(m_generateFlightPlanAction);
    m_toolsMenu->addAction(m_clearMapAction);

    m_viewMenu = menuBar()->addMenu(tr("&View"));

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAction = m_helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::createToolbars()
{
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->addAction(m_newProjectAction);
    m_mainToolBar->addAction(m_openProjectAction);
    m_mainToolBar->addAction(m_saveProjectAction);

    m_mapToolBar = addToolBar(tr("Flight Planning"));
    m_mapToolBar->addAction(m_generateFlightPlanAction);
    m_mapToolBar->addAction(m_exportKMZAction);
    m_mapToolBar->addSeparator();
    m_mapToolBar->addAction(m_clearMapAction);
}

void MainWindow::createDockWidgets()
{
    m_projectDock = new QDockWidget(tr("Project Explorer"), this);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);

    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);

    // Add dock widgets to view menu
    m_viewMenu->addAction(m_projectDock->toggleViewAction());
    m_viewMenu->addAction(m_propertiesDock->toggleViewAction());
}

void MainWindow::newProject()
{
    QString name = "New Project";  // Would show dialog
    QString basePath = Core::Settings::instance().defaultProjectPath();

    auto* project = Core::ProjectManager::instance().createProject(name, basePath);
    if (project) {
        statusBar()->showMessage(tr("Project created: %1").arg(name), 5000);
    } else {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to create project: %1")
            .arg(Core::ProjectManager::instance().lastError()));
    }
}

void MainWindow::openProject()
{
    QString fileName = QFileDialog::getExistingDirectory(
        this,
        tr("Open Project"),
        Core::Settings::instance().defaultProjectPath()
    );

    if (!fileName.isEmpty()) {
        if (Core::ProjectManager::instance().openProject(fileName)) {
            statusBar()->showMessage(tr("Project opened: %1").arg(fileName), 5000);
        }
    }
}

void MainWindow::saveProject()
{
    if (Core::ProjectManager::instance().saveProject()) {
        statusBar()->showMessage(tr("Project saved"), 5000);
    } else {
        QMessageBox::warning(this, tr("Warning"),
            tr("Failed to save project: %1")
            .arg(Core::ProjectManager::instance().lastError()));
    }
}

void MainWindow::exit()
{
    close();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About DroneMapper"),
        tr("<h2>DroneMapper v1.0</h2>"
           "<p>Professional Drone Flight Planning & Photogrammetry Application</p>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Flight planning with KMZ export for DJI drones</li>"
           "<li>Photogrammetry processing</li>"
           "<li>Professional mapping and analysis tools</li>"
           "</ul>"));
}

void MainWindow::readSettings()
{
    auto& settings = Core::Settings::instance();
    restoreGeometry(settings.mainWindowGeometry());
    restoreState(settings.mainWindowState());
}

void MainWindow::writeSettings()
{
    auto& settings = Core::Settings::instance();
    settings.setMainWindowGeometry(saveGeometry());
    settings.setMainWindowState(saveState());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Core::ProjectManager::instance().closeProject();
    writeSettings();
    event->accept();
}

void MainWindow::onAreaSelected(const QString& geojson)
{
    m_currentAreaGeoJson = geojson;
    m_generateFlightPlanAction->setEnabled(true);
    statusBar()->showMessage(tr("Area selected - Click 'Generate Flight Plan' or press Ctrl+G"), 0);
}

void MainWindow::onFlightPlanRequested()
{
    onGenerateFlightPlan();
}

void MainWindow::onGenerateFlightPlan()
{
    if (m_currentAreaGeoJson.isEmpty()) {
        QMessageBox::warning(this, tr("No Area Selected"),
            tr("Please draw an area on the map first."));
        return;
    }

    // Parse GeoJSON to get polygon coordinates
    QJsonDocument doc = QJsonDocument::fromJson(m_currentAreaGeoJson.toUtf8());
    QJsonObject obj = doc.object();

    if (obj["type"].toString() != "Feature") {
        QMessageBox::critical(this, tr("Error"), tr("Invalid GeoJSON format"));
        return;
    }

    QJsonObject geometry = obj["geometry"].toObject();
    QString geomType = geometry["type"].toString();

    QPolygonF polygon;
    QJsonArray coordinates;

    if (geomType == "Polygon") {
        coordinates = geometry["coordinates"].toArray()[0].toArray();
    } else if (geomType == "Rectangle" || geomType == "Circle") {
        // Handle rectangle/circle - convert to polygon
        coordinates = geometry["coordinates"].toArray()[0].toArray();
    } else {
        QMessageBox::critical(this, tr("Error"),
            tr("Unsupported geometry type: %1").arg(geomType));
        return;
    }

    // Convert coordinates to polygon
    for (const QJsonValue& coord : coordinates) {
        QJsonArray point = coord.toArray();
        if (point.size() >= 2) {
            polygon.append(QPointF(point[0].toDouble(), point[1].toDouble()));
        }
    }

    if (polygon.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to parse polygon coordinates"));
        return;
    }

    // Calculate polygon area (rough approximation in square meters)
    // Convert lat/lon polygon to approximate area
    double area = 0.0;
    for (int i = 0; i < polygon.count(); ++i) {
        int j = (i + 1) % polygon.count();
        area += polygon[i].x() * polygon[j].y();
        area -= polygon[j].x() * polygon[i].y();
    }
    area = std::abs(area) / 2.0;
    // Convert from square degrees to square meters (approximate)
    // At equator, 1 degree ~ 111km, so 1 sq degree ~ 12321 km² = 12,321,000,000 m²
    // This is very rough - proper calculation would use actual lat/lon
    double avgLat = 0.0;
    for (const auto& pt : polygon) {
        avgLat += pt.y();
    }
    avgLat /= polygon.count();
    double metersPerDegree = 111000.0 * std::cos(avgLat * M_PI / 180.0);
    area *= metersPerDegree * metersPerDegree;

    // Show mission parameters dialog
    MissionParametersDialog dialog(this);
    dialog.setSurveyAreaSize(area);

    if (dialog.exec() != QDialog::Accepted) {
        return; // User cancelled
    }

    // Get parameters from dialog
    Models::MissionParameters params = dialog.parameters();

    // Create flight plan
    Models::FlightPlan plan;
    plan.setName("Flight Plan " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    plan.setPatternType(dialog.patternType());

    // Copy parameters to plan
    plan.parameters() = params;

    // Get spacing from parameters (already calculated in dialog)
    double spacing = params.pathSpacing();

    statusBar()->showMessage(tr("Generating flight plan..."), 0);

    // Generate coverage pattern
    Geospatial::CoveragePatternGenerator generator;
    auto waypoints = generator.generateParallelLines(
        polygon,
        params.flightAltitude(),
        params.flightDirection(),
        spacing,
        params.frontOverlap() / 100.0);

    if (waypoints.isEmpty()) {
        QMessageBox::warning(this, tr("Generation Failed"),
            tr("Failed to generate waypoints. Check polygon validity."));
        statusBar()->showMessage(tr("Flight plan generation failed"), 5000);
        return;
    }

    // Add waypoints to flight plan
    for (const auto& waypoint : waypoints) {
        plan.addWaypoint(waypoint);
    }

    // Store the survey area
    plan.setSurveyArea(polygon);

    // Calculate statistics
    double totalDistance = 0.0;
    for (int i = 1; i < waypoints.count(); ++i) {
        totalDistance += Geospatial::GeoUtils::distanceBetween(
            waypoints[i-1].coordinate(), waypoints[i].coordinate());
    }

    double avgSpeed = plan.parameters().flightSpeed();
    int flightTime = static_cast<int>(totalDistance / avgSpeed);
    int photoCount = waypoints.count() * 3; // Approximate

    // Display on map
    m_mapWidget->displayFlightPath(plan);
    m_mapWidget->updateFlightInfo(totalDistance, flightTime, photoCount);

    // Store flight plan for export
    if (m_currentFlightPlan) {
        delete m_currentFlightPlan;
    }
    m_currentFlightPlan = new Models::FlightPlan(plan);

    // Enable export
    m_exportKMZAction->setEnabled(true);

    statusBar()->showMessage(
        tr("Flight plan generated: %1 waypoints, %2m distance, %3min flight time")
        .arg(waypoints.count())
        .arg(QString::number(totalDistance, 'f', 0))
        .arg(flightTime / 60),
        10000);
}

void MainWindow::onClearMap()
{
    m_mapWidget->clearMap();
    m_currentAreaGeoJson.clear();

    if (m_currentFlightPlan) {
        delete m_currentFlightPlan;
        m_currentFlightPlan = nullptr;
    }

    m_generateFlightPlanAction->setEnabled(false);
    m_exportKMZAction->setEnabled(false);
    statusBar()->showMessage(tr("Map cleared"), 3000);
}

void MainWindow::onExportKMZ()
{
    if (!m_currentFlightPlan) {
        QMessageBox::warning(this, tr("No Flight Plan"),
            tr("Please generate a flight plan first."));
        return;
    }

    // Ask user to select drone model
    QStringList droneModels;
    droneModels << tr("DJI Mini 3")
                << tr("DJI Mini 3 Pro")
                << tr("DJI Air 3")
                << tr("DJI Mavic 3")
                << tr("DJI Mavic 3 Pro");

    bool ok;
    QString selectedModel = QInputDialog::getItem(this,
        tr("Select Drone Model"),
        tr("Choose your DJI drone model for KMZ export:"),
        droneModels,
        0, // Default to Mini 3
        false, // Not editable
        &ok);

    if (!ok) {
        return; // User cancelled
    }

    // Map selection to DroneModel enum
    Geospatial::WPMLWriter::DroneModel droneModel;
    if (selectedModel.contains("Mini 3 Pro")) {
        droneModel = Geospatial::WPMLWriter::DroneModel::Mini3Pro;
    } else if (selectedModel.contains("Mini 3")) {
        droneModel = Geospatial::WPMLWriter::DroneModel::Mini3;
    } else if (selectedModel.contains("Air 3")) {
        droneModel = Geospatial::WPMLWriter::DroneModel::Air3;
    } else if (selectedModel.contains("Mavic 3 Pro")) {
        droneModel = Geospatial::WPMLWriter::DroneModel::Mavic3Pro;
    } else {
        droneModel = Geospatial::WPMLWriter::DroneModel::Mavic3;
    }

    // Get save file path
    QString defaultName = QString("flight_plan_%1.kmz")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export KMZ Flight Plan"),
        QDir::homePath() + "/" + defaultName,
        tr("KMZ Files (*.kmz)"));

    if (fileName.isEmpty()) {
        return; // User cancelled
    }

    // Ensure .kmz extension
    if (!fileName.endsWith(".kmz", Qt::CaseInsensitive)) {
        fileName += ".kmz";
    }

    // Generate and save KMZ
    statusBar()->showMessage(tr("Generating KMZ file..."), 0);

    Geospatial::KMZGenerator generator;
    if (generator.generate(*m_currentFlightPlan, fileName, droneModel)) {
        QMessageBox::information(this, tr("Export Successful"),
            tr("KMZ file created successfully!\n\n"
               "File: %1\n\n"
               "To use this flight plan:\n"
               "1. Copy the KMZ file to your mobile device\n"
               "2. Open DJI Fly app\n"
               "3. Go to Create tab\n"
               "4. Tap the '+' button\n"
               "5. Select 'Import' and choose this KMZ file\n"
               "6. Review the mission and fly safely!\n\n"
               "Waypoints: %2\n"
               "Drone Model: %3")
            .arg(fileName)
            .arg(m_currentFlightPlan->waypointCount())
            .arg(selectedModel));

        statusBar()->showMessage(
            tr("KMZ exported successfully to %1").arg(QFileInfo(fileName).fileName()),
            10000);
    } else {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Failed to generate KMZ file.\n\nError: %1")
            .arg(generator.lastError()));

        statusBar()->showMessage(tr("KMZ export failed"), 5000);
    }
}

} // namespace UI
} // namespace DroneMapper
