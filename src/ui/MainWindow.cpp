#include "MainWindow.h"
#include "Settings.h"
#include "ProjectManager.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>

namespace DroneMapper {
namespace UI {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("DroneMapper - Flight Planning & Photogrammetry");
    resize(1280, 800);

    createActions();
    createMenus();
    createToolbars();
    createDockWidgets();
    readSettings();

    statusBar()->showMessage("Ready", 5000);
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
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_newProjectAction);
    m_fileMenu->addAction(m_openProjectAction);
    m_fileMenu->addAction(m_saveProjectAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
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

} // namespace UI
} // namespace DroneMapper
