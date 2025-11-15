#include "MainWindow.h"
#include "Logger.h"
#include "DatabaseManager.h"
#include "Settings.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setOrganizationName("DroneMapper");
    QApplication::setApplicationName("DroneMapper");
    QApplication::setApplicationVersion("1.0.0");

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
    mainWindow.show();

    LOG_INFO("Application ready");

    return app.exec();
}
