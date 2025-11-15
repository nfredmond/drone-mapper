#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>

namespace DroneMapper {
namespace UI {

class MapWidget;

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

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_toolsMenu;
    QMenu *m_helpMenu;

    // Toolbars
    QToolBar *m_mainToolBar;
    QToolBar *m_mapToolBar;

    // Dock widgets
    QDockWidget *m_projectDock;
    QDockWidget *m_propertiesDock;

    // Central widget
    MapWidget *m_mapWidget;

    // Current state
    QString m_currentAreaGeoJson;
};

} // namespace UI
} // namespace DroneMapper

#endif // MAINWINDOW_H
