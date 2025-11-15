#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace DroneMapper {
namespace Core {

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager()
    : m_initialized(false)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    if (!createTables()) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool DatabaseManager::createTables()
{
    QStringList queries;

    // Projects table
    queries << R"(
        CREATE TABLE IF NOT EXISTS projects (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            project_path TEXT NOT NULL,
            status INTEGER,
            created_date TEXT,
            modified_date TEXT,
            image_count INTEGER DEFAULT 0
        )
    )";

    // Flight plans table
    queries << R"(
        CREATE TABLE IF NOT EXISTS flight_plans (
            id TEXT PRIMARY KEY,
            project_id TEXT,
            name TEXT NOT NULL,
            description TEXT,
            pattern_type INTEGER,
            created_date TEXT,
            modified_date TEXT,
            mission_data BLOB,
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
        )
    )";

    // Settings table
    queries << R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )";

    for (const QString& queryStr : queries) {
        if (!executeSql(queryStr)) {
            return false;
        }
    }

    return true;
}

bool DatabaseManager::executeSql(const QString& sql)
{
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qWarning() << "SQL Error:" << m_lastError;
        return false;
    }
    return true;
}

bool DatabaseManager::saveProject(const Models::Project& project)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO projects
        (id, name, description, project_path, status, created_date, modified_date, image_count)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(project.id());
    query.addBindValue(project.name());
    query.addBindValue(project.description());
    query.addBindValue(project.projectPath());
    query.addBindValue(static_cast<int>(project.status()));
    query.addBindValue(project.createdDate().toString(Qt::ISODate));
    query.addBindValue(project.modifiedDate().toString(Qt::ISODate));
    query.addBindValue(project.imageCount());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::loadProject(const QString& projectId, Models::Project& project)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM projects WHERE id = ?");
    query.addBindValue(projectId);

    if (!query.exec() || !query.next()) {
        m_lastError = "Project not found";
        return false;
    }

    // Load project data from query result
    // Note: This is a simplified implementation
    // Full implementation would deserialize all fields

    return true;
}

QList<Models::Project> DatabaseManager::loadAllProjects()
{
    QList<Models::Project> projects;

    QSqlQuery query("SELECT * FROM projects ORDER BY modified_date DESC", m_database);

    while (query.next()) {
        // Create project from query data
        // Simplified - full implementation would deserialize all fields
        Models::Project project;
        projects.append(project);
    }

    return projects;
}

bool DatabaseManager::deleteProject(const QString& projectId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM projects WHERE id = ?");
    query.addBindValue(projectId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::saveSetting(const QString& key, const QVariant& value)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)");
    query.addBindValue(key);
    query.addBindValue(value.toString());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QVariant DatabaseManager::loadSetting(const QString& key, const QVariant& defaultValue)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT value FROM settings WHERE key = ?");
    query.addBindValue(key);

    if (query.exec() && query.next()) {
        return query.value(0);
    }

    return defaultValue;
}

// Stub implementations for flight plan methods
bool DatabaseManager::saveFlightPlan(const QString& projectId, const Models::FlightPlan& plan)
{
    // TODO: Implement
    return true;
}

bool DatabaseManager::loadFlightPlan(const QString& planId, Models::FlightPlan& plan)
{
    // TODO: Implement
    return false;
}

QList<Models::FlightPlan> DatabaseManager::loadFlightPlansForProject(const QString& projectId)
{
    // TODO: Implement
    return QList<Models::FlightPlan>();
}

bool DatabaseManager::deleteFlightPlan(const QString& planId)
{
    // TODO: Implement
    return true;
}

} // namespace Core
} // namespace DroneMapper
