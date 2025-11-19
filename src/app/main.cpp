#include "MainWindow.h"
#include "Logger.h"
#include "DatabaseManager.h"
#include "Settings.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QStyleFactory>
#include <QPalette>

void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    app.setPalette(darkPalette);

    app.setStyleSheet(
        "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }"
        "QDockWidget::title { background: #2d2d2d; padding: 5px; text-align: center; }"
        "QMainWindow::separator { background-color: #2d2d2d; width: 1px; height: 1px; }"
        "QMenuBar::item { padding: 5px 10px; background: transparent; }"
        "QMenuBar::item:selected { background: #2a82da; }"
        "QMenu::item { padding: 5px 30px 5px 20px; }"
        "QMenu::item:selected { background-color: #2a82da; }"
        "QStatusBar { background: #2d2d2d; color: #a0a0a0; }"
        "QPushButton { padding: 5px 10px; border: 1px solid #5c5c5c; border-radius: 3px; background-color: #353535; }"
        "QPushButton:hover { background-color: #454545; }"
        "QPushButton:pressed { background-color: #2a82da; }"
        "QLineEdit { padding: 3px; border: 1px solid #5c5c5c; border-radius: 3px; background-color: #1e1e1e; }"
        "QComboBox { padding: 3px; border: 1px solid #5c5c5c; border-radius: 3px; background-color: #353535; }"
        "QTabWidget::pane { border: 1px solid #444; }"
    );
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setOrganizationName("DroneMapper");
    QApplication::setApplicationName("DroneMapper");
    QApplication::setApplicationVersion("1.0.0");

    // Apply Professional Dark Theme
    applyDarkTheme(app);

    // Initialize logging
    DroneMapper::Core::Logger::instance().setLogLevel(DroneMapper::Core::Logger::Level::Info);
    LOG_INFO("DroneMapper starting...");

    // Initialize database
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    QString dbPath = QDir(appDataPath).filePath("dronemapper.db");

    if (!DroneMapper::Core::DatabaseManager::instance().initialize(dbPath)) {
        LOG_ERROR("Failed to initialize database: " +
                  DroneMapper::Core::DatabaseManager::instance().lastError());
        return 1;
    }

    LOG_INFO("Database initialized: " + dbPath);

    // Create and show main window
    DroneMapper::UI::MainWindow mainWindow;
    mainWindow.showMaximized(); // Start maximized for professional feel

    LOG_INFO("Application ready");

    return app.exec();
}
