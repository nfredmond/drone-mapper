#include "MainWindow.h"
#include "MapWidget.h"
#include "MissionParametersDialog.h"
#include "WeatherWidget.h"
#include "WindOverlayWidget.h"
#include "TerrainElevationViewer.h"
#include "PointCloudViewer.h"
#include "SimulationPreviewWidget.h"
#include "Settings.h"
#include "ReportGenerator.h"
#include "ProjectManager.h"
#include "WeatherService.h"
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
#include <QVBoxLayout>
#include <QDesktopServices>
#include <cmath>

namespace DroneMapper {
namespace UI {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mapWidget(new MapWidget(this))
    , m_weatherWidget(nullptr)
    , m_windOverlay(nullptr)
    , m_viewersTab(nullptr)
    , m_terrainViewer(nullptr)
    , m_pointCloudViewer(nullptr)
    , m_simulationPreview(nullptr)
    , m_currentFlightPlan(nullptr)
{
    setWindowTitle("DroneMapper - Professional Flight Planning & Photogrammetry");
    resize(1400, 900);

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

    // Advanced feature actions
    m_showWindOverlayAction = new QAction(tr("Show &Wind Overlay"), this);
    m_showWindOverlayAction->setCheckable(true);
    connect(m_showWindOverlayAction, &QAction::triggered, this, &MainWindow::onShowWindOverlay);

    m_showTerrainViewerAction = new QAction(tr("Show &Terrain Viewer"), this);
    m_showTerrainViewerAction->setShortcut(QKeySequence(tr("Ctrl+T")));
    connect(m_showTerrainViewerAction, &QAction::triggered, this, &MainWindow::onShowTerrainViewer);

    m_showPointCloudViewerAction = new QAction(tr("Show &Point Cloud Viewer"), this);
    m_showPointCloudViewerAction->setShortcut(QKeySequence(tr("Ctrl+P")));
    connect(m_showPointCloudViewerAction, &QAction::triggered, this, &MainWindow::onShowPointCloudViewer);

    m_runCOLMAPAction = new QAction(tr("Run COLMAP &Reconstruction..."), this);
    m_runCOLMAPAction->setShortcut(QKeySequence(tr("Ctrl+R")));
    connect(m_runCOLMAPAction, &QAction::triggered, this, &MainWindow::onRunCOLMAPReconstruction);

    m_showWeatherPanelAction = new QAction(tr("Show &Weather Panel"), this);
    m_showWeatherPanelAction->setCheckable(true);
    connect(m_showWeatherPanelAction, &QAction::triggered, this, &MainWindow::onShowWeatherPanel);

    m_toggle3DViewersAction = new QAction(tr("Toggle &3D Viewers"), this);
    m_toggle3DViewersAction->setShortcut(QKeySequence(tr("F3")));
    connect(m_toggle3DViewersAction, &QAction::triggered, this, &MainWindow::onToggle3DViewers);

    m_loadPointCloudAction = new QAction(tr("Load Point &Cloud..."), this);
    connect(m_loadPointCloudAction, &QAction::triggered, this, &MainWindow::onLoadPointCloud);

    m_loadDEMAction = new QAction(tr("Load &DEM/Terrain..."), this);
    connect(m_loadDEMAction, &QAction::triggered, this, &MainWindow::onLoadDEM);

    // Mission actions
    m_previewMissionAction = new QAction(tr("&Preview Mission Simulation"), this);
    m_previewMissionAction->setShortcut(QKeySequence(tr("Ctrl+Shift+P")));
    m_previewMissionAction->setEnabled(false);  // Enabled when flight plan exists
    connect(m_previewMissionAction, &QAction::triggered, this, &MainWindow::onPreviewMission);

    m_generateReportAction = new QAction(tr("Generate Mission &Report..."), this);
    m_generateReportAction->setShortcut(QKeySequence(tr("Ctrl+Shift+R")));
    m_generateReportAction->setEnabled(false);  // Enabled when flight plan exists
    connect(m_generateReportAction, &QAction::triggered, this, &MainWindow::onGenerateReport);
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

    m_missionMenu = menuBar()->addMenu(tr("&Mission"));
    m_missionMenu->addAction(m_previewMissionAction);
    m_missionMenu->addAction(m_generateReportAction);

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_showWeatherPanelAction);
    m_viewMenu->addAction(m_toggle3DViewersAction);
    m_viewMenu->addSeparator();

    m_visualizationMenu = menuBar()->addMenu(tr("&Visualization"));
    m_visualizationMenu->addAction(m_showWindOverlayAction);
    m_visualizationMenu->addAction(m_showTerrainViewerAction);
    m_visualizationMenu->addAction(m_showPointCloudViewerAction);
    m_visualizationMenu->addSeparator();
    m_visualizationMenu->addAction(m_loadDEMAction);
    m_visualizationMenu->addAction(m_loadPointCloudAction);

    m_photogrammetryMenu = menuBar()->addMenu(tr("&Photogrammetry"));
    m_photogrammetryMenu->addAction(m_runCOLMAPAction);

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

    m_visualizationToolBar = addToolBar(tr("Visualization"));
    m_visualizationToolBar->addAction(m_showTerrainViewerAction);
    m_visualizationToolBar->addAction(m_showPointCloudViewerAction);
    m_visualizationToolBar->addSeparator();
    m_visualizationToolBar->addAction(m_runCOLMAPAction);
}

void MainWindow::createDockWidgets()
{
    m_projectDock = new QDockWidget(tr("Project Explorer"), this);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);

    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);

    // Weather dock
    m_weatherDock = new QDockWidget(tr("Weather"), this);
    m_weatherDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    m_weatherWidget = new WeatherWidget(this);
    m_weatherDock->setWidget(m_weatherWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_weatherDock);
    m_weatherDock->hide(); // Hidden by default

    // 3D Viewers dock
    m_viewersDock = new QDockWidget(tr("3D Viewers"), this);
    m_viewersDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_viewersTab = new QTabWidget(this);

    m_terrainViewer = new TerrainElevationViewer(this);
    m_pointCloudViewer = new PointCloudViewer(this);

    m_viewersTab->addTab(m_terrainViewer, tr("Terrain"));
    m_viewersTab->addTab(m_pointCloudViewer, tr("Point Cloud"));

    m_viewersDock->setWidget(m_viewersTab);
    addDockWidget(Qt::BottomDockWidgetArea, m_viewersDock);
    m_viewersDock->hide(); // Hidden by default

    // Add dock widgets to view menu
    m_viewMenu->addAction(m_projectDock->toggleViewAction());
    m_viewMenu->addAction(m_propertiesDock->toggleViewAction());
    m_viewMenu->addAction(m_weatherDock->toggleViewAction());
    m_viewMenu->addAction(m_viewersDock->toggleViewAction());
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

    // Enable export and mission actions
    m_exportKMZAction->setEnabled(true);
    m_previewMissionAction->setEnabled(true);
    m_generateReportAction->setEnabled(true);

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
    m_previewMissionAction->setEnabled(false);
    m_generateReportAction->setEnabled(false);
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

// Advanced feature implementations

void MainWindow::onShowWindOverlay()
{
    if (!m_windOverlay) {
        m_windOverlay = new WindOverlayWidget(m_mapWidget);
        m_windOverlay->setWeatherService(&Core::WeatherService::instance());
        m_windOverlay->resize(m_mapWidget->size());
        m_windOverlay->show();

        // Update wind overlay when flight plan is generated
        if (m_currentFlightPlan) {
            auto bounds = m_currentFlightPlan->surveyArea().boundingRect();
            Models::GeospatialCoordinate topLeft(bounds.top(), bounds.left(), 0);
            Models::GeospatialCoordinate bottomRight(bounds.bottom(), bounds.right(), 0);
            m_windOverlay->setViewBounds(topLeft, bottomRight);
            m_windOverlay->refreshData();
        }
    }

    m_windOverlay->setVisible(m_showWindOverlayAction->isChecked());
    statusBar()->showMessage(
        m_showWindOverlayAction->isChecked() ? tr("Wind overlay enabled") : tr("Wind overlay disabled"),
        3000);
}

void MainWindow::onShowTerrainViewer()
{
    if (m_viewersDock->isHidden()) {
        m_viewersDock->show();
    }
    m_viewersTab->setCurrentWidget(m_terrainViewer);

    // Load flight plan if available
    if (m_currentFlightPlan) {
        m_terrainViewer->setFlightPlan(*m_currentFlightPlan);
    }

    statusBar()->showMessage(tr("Terrain viewer opened"), 3000);
}

void MainWindow::onShowPointCloudViewer()
{
    if (m_viewersDock->isHidden()) {
        m_viewersDock->show();
    }
    m_viewersTab->setCurrentWidget(m_pointCloudViewer);

    statusBar()->showMessage(tr("Point cloud viewer opened - Use File → Load Point Cloud to load data"), 5000);
}

void MainWindow::onRunCOLMAPReconstruction()
{
    QString imageDir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Image Directory for COLMAP Reconstruction"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly);

    if (imageDir.isEmpty()) {
        return;
    }

    QMessageBox::information(this, tr("COLMAP Reconstruction"),
        tr("COLMAP reconstruction will process images in:\n%1\n\n"
           "This feature requires COLMAP to be installed.\n"
           "The reconstruction will run in the background.\n\n"
           "Results will be available in the Point Cloud Viewer when complete.")
        .arg(imageDir));

    // TODO: Show COLMAP configuration dialog
    // TODO: Start COLMAP processing
    // TODO: Show progress dialog

    statusBar()->showMessage(tr("COLMAP reconstruction queued for: %1").arg(QFileInfo(imageDir).fileName()), 5000);
}

void MainWindow::onShowWeatherPanel()
{
    if (m_weatherDock->isHidden()) {
        m_weatherDock->show();

        // Set location for weather if we have a flight plan
        if (m_currentFlightPlan && !m_currentFlightPlan->waypoints().isEmpty()) {
            auto firstWaypoint = m_currentFlightPlan->waypoints().first();
            m_weatherWidget->setLocation(
                firstWaypoint.coordinate().latitude(),
                firstWaypoint.coordinate().longitude());
            m_weatherWidget->refreshWeather();
        }
    } else {
        m_weatherDock->hide();
    }

    m_showWeatherPanelAction->setChecked(!m_weatherDock->isHidden());
}

void MainWindow::onToggle3DViewers()
{
    if (m_viewersDock->isHidden()) {
        m_viewersDock->show();
        statusBar()->showMessage(tr("3D Viewers shown"), 3000);
    } else {
        m_viewersDock->hide();
        statusBar()->showMessage(tr("3D Viewers hidden"), 3000);
    }
}

void MainWindow::onLoadPointCloud()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Load Point Cloud"),
        QDir::homePath(),
        tr("Point Cloud Files (*.ply *.las *.laz *.xyz *.txt);;PLY Files (*.ply);;LAS Files (*.las *.laz);;XYZ Files (*.xyz *.txt);;All Files (*.*)"));

    if (fileName.isEmpty()) {
        return;
    }

    statusBar()->showMessage(tr("Loading point cloud..."), 0);

    if (m_pointCloudViewer->loadPointCloud(fileName)) {
        onShowPointCloudViewer();
        statusBar()->showMessage(
            tr("Point cloud loaded: %1").arg(QFileInfo(fileName).fileName()),
            5000);
    } else {
        QMessageBox::critical(this, tr("Load Error"),
            tr("Failed to load point cloud from:\n%1").arg(fileName));
        statusBar()->showMessage(tr("Failed to load point cloud"), 5000);
    }
}

void MainWindow::onLoadDEM()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Load DEM/Terrain Data"),
        QDir::homePath(),
        tr("DEM Files (*.tif *.tiff *.hgt);;GeoTIFF (*.tif *.tiff);;SRTM (*.hgt);;All Files (*.*)"));

    if (fileName.isEmpty()) {
        return;
    }

    statusBar()->showMessage(tr("Loading DEM..."), 0);

    if (m_terrainViewer->loadDEM(fileName)) {
        onShowTerrainViewer();
        statusBar()->showMessage(
            tr("DEM loaded: %1").arg(QFileInfo(fileName).fileName()),
            5000);
    } else {
        QMessageBox::critical(this, tr("Load Error"),
            tr("Failed to load DEM from:\n%1").arg(fileName));
        statusBar()->showMessage(tr("Failed to load DEM"), 5000);
    }
}

void MainWindow::onPreviewMission()
{
    if (!m_currentFlightPlan) {
        QMessageBox::warning(this, tr("No Flight Plan"),
            tr("Please generate a flight plan first."));
        return;
    }

    if (!m_simulationPreview) {
        m_simulationPreview = new SimulationPreviewWidget(this);
        connect(m_simulationPreview, &SimulationPreviewWidget::closed,
                [this]() { m_simulationPreview->deleteLater(); m_simulationPreview = nullptr; });
    }

    m_simulationPreview->loadFlightPlan(*m_currentFlightPlan);
    m_simulationPreview->show();
    m_simulationPreview->raise();
    m_simulationPreview->activateWindow();

    statusBar()->showMessage(tr("Mission simulation preview opened"), 3000);
}

void MainWindow::onGenerateReport()
{
    if (!m_currentFlightPlan) {
        QMessageBox::warning(this, tr("No Flight Plan"),
            tr("Please generate a flight plan first."));
        return;
    }

    // Ask user for report format
    QStringList formats;
    formats << "HTML" << "Markdown" << "PDF (HTML-based)";

    bool ok;
    QString format = QInputDialog::getItem(this, tr("Select Report Format"),
        tr("Choose report format:"), formats, 0, false, &ok);

    if (!ok || format.isEmpty()) {
        return;
    }

    // Get output file path
    QString filter;
    QString defaultExt;
    Core::ReportFormat reportFormat;

    if (format.startsWith("HTML")) {
        filter = tr("HTML Files (*.html)");
        defaultExt = ".html";
        reportFormat = Core::ReportFormat::HTML;
    } else if (format.startsWith("Markdown")) {
        filter = tr("Markdown Files (*.md)");
        defaultExt = ".md";
        reportFormat = Core::ReportFormat::Markdown;
    } else {
        filter = tr("HTML Files (*.html)");
        defaultExt = ".html";
        reportFormat = Core::ReportFormat::HTML;  // PDF via HTML for now
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Mission Report"),
        QDir::homePath() + "/mission_report" + defaultExt,
        filter);

    if (fileName.isEmpty()) {
        return;
    }

    // Set up report options
    Core::ReportOptions options;
    options.companyName = "DroneMapper Professional";
    options.projectName = "Mission Report";
    options.sections.includeCoverPage = true;
    options.sections.includeMissionOverview = true;
    options.sections.includeFlightPathMap = true;
    options.sections.includeStatistics = true;
    options.sections.includeCostBreakdown = true;
    options.sections.includeWeatherAnalysis = true;
    options.sections.includeSafetyAnalysis = true;
    options.sections.includeEquipment = true;
    options.sections.includePhotogrammetryPlan = true;

    // Generate report
    Core::ReportGenerator generator;
    statusBar()->showMessage(tr("Generating report..."), 0);

    if (generator.generateReport(*m_currentFlightPlan, fileName, reportFormat, options)) {
        statusBar()->showMessage(tr("Report generated successfully: %1").arg(fileName), 5000);

        // Ask if user wants to open the report
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Report Generated"),
            tr("Report generated successfully!\n\nWould you like to open it?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
        }
    } else {
        QMessageBox::critical(this, tr("Report Error"),
            tr("Failed to generate report:\n%1").arg(generator.lastError()));
        statusBar()->showMessage(tr("Failed to generate report"), 5000);
    }
}

} // namespace UI
} // namespace DroneMapper
