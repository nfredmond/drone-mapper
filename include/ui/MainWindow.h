#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>

namespace DroneMapper {
namespace UI {

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

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;

    // Toolbars
    QToolBar *m_mainToolBar;

    // Dock widgets
    QDockWidget *m_projectDock;
    QDockWidget *m_propertiesDock;
};

} // namespace UI
} // namespace DroneMapper

#endif // MAINWINDOW_H
