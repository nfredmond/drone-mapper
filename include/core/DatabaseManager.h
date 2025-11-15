#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "Project.h"
#include "FlightPlan.h"
#include <QString>
#include <QSqlDatabase>
#include <QList>

namespace DroneMapper {
namespace Core {

/**
 * @brief Manages SQLite database for projects and flight plans
 */
class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initialize(const QString& dbPath);
    bool isInitialized() const { return m_initialized; }

    // Project operations
    bool saveProject(const Models::Project& project);
    bool loadProject(const QString& projectId, Models::Project& project);
    QList<Models::Project> loadAllProjects();
    bool deleteProject(const QString& projectId);

    // Flight plan operations
    bool saveFlightPlan(const QString& projectId, const Models::FlightPlan& plan);
    bool loadFlightPlan(const QString& planId, Models::FlightPlan& plan);
    QList<Models::FlightPlan> loadFlightPlansForProject(const QString& projectId);
    bool deleteFlightPlan(const QString& planId);

    // Settings
    bool saveSetting(const QString& key, const QVariant& value);
    QVariant loadSetting(const QString& key, const QVariant& defaultValue = QVariant());

    QString lastError() const { return m_lastError; }

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createTables();
    bool executeSql(const QString& sql);

    QSqlDatabase m_database;
    bool m_initialized;
    QString m_lastError;
};

} // namespace Core
} // namespace DroneMapper

#endif // DATABASEMANAGER_H
