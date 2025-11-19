#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QProgressDialog>

namespace DroneMapper {
namespace Models {
    class FlightPlan;
}
namespace Core {
    class WeatherService;
}
namespace Photogrammetry {
    class COLMAPIntegration;
}
namespace UI {

class MapWidget;
class WeatherWidget;
class WindOverlayWidget;
class TerrainElevationViewer;
class PointCloudViewer;
class SimulationPreviewWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void exit();
    void about();

    // Map interaction slots
    void onAreaSelected(const QString& geojson);
    void onFlightPlanRequested();
    void onGenerateFlightPlan();
    void onClearMap();
    void onExportKMZ();

    // Advanced feature slots
    void onShowWindOverlay();
    void onShowTerrainViewer();
    void onShowPointCloudViewer();
    void onRunCOLMAPReconstruction();
    void onShowWeatherPanel();
    void onToggle3DViewers();
    void onLoadPointCloud();
    void onLoadDEM();
    void onPreviewMission();
    void onGenerateReport();

    // COLMAP slots
    void onCOLMAPProgress(double progress, const QString& message);
    void onCOLMAPFinished();
    void onCOLMAPError(const QString& error);

private:
    void createActions();
    void createMenus();
    void createToolbars();
    void createDockWidgets();
    void readSettings();
    void writeSettings();

    void closeEvent(QCloseEvent *event) override;

    // Actions
    QAction *m_newProjectAction;
    QAction *m_openProjectAction;
    QAction *m_saveProjectAction;
    QAction *m_exitAction;
    QAction *m_generateFlightPlanAction;
    QAction *m_exportKMZAction;
    QAction *m_clearMapAction;

    // Advanced feature actions
    QAction *m_showWindOverlayAction;
    QAction *m_showTerrainViewerAction;
    QAction *m_showPointCloudViewerAction;
    QAction *m_runCOLMAPAction;
    QAction *m_showWeatherPanelAction;
    QAction *m_toggle3DViewersAction;
    QAction *m_loadPointCloudAction;
    QAction *m_loadDEMAction;
    QAction *m_previewMissionAction;
    QAction *m_generateReportAction;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_missionMenu;
    QMenu *m_toolsMenu;
    QMenu *m_visualizationMenu;
    QMenu *m_photogrammetryMenu;
    QMenu *m_helpMenu;

    // Toolbars
    QToolBar *m_mainToolBar;
    QToolBar *m_mapToolBar;
    QToolBar *m_visualizationToolBar;

    // Dock widgets
    QDockWidget *m_projectDock;
    QDockWidget *m_propertiesDock;
    QDockWidget *m_weatherDock;
    QDockWidget *m_viewersDock;

    // Central widget
    MapWidget *m_mapWidget;

    // Advanced widgets
    WeatherWidget *m_weatherWidget;
    WindOverlayWidget *m_windOverlay;
    QTabWidget *m_viewersTab;
    TerrainElevationViewer *m_terrainViewer;
    PointCloudViewer *m_pointCloudViewer;
    SimulationPreviewWidget *m_simulationPreview;

    // COLMAP Integration
    Photogrammetry::COLMAPIntegration* m_colmapIntegration;
    QProgressDialog* m_progressDialog;

    // Current state
    QString m_currentAreaGeoJson;
    Models::FlightPlan *m_currentFlightPlan;
};

} // namespace UI
} // namespace DroneMapper

#endif // MAINWINDOW_H
