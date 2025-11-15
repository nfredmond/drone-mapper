#include "ProjectManager.h"
#include "DatabaseManager.h"
#include <QDir>
#include <QStandardPaths>

namespace DroneMapper {
namespace Core {

ProjectManager& ProjectManager::instance()
{
    static ProjectManager instance;
    return instance;
}

ProjectManager::ProjectManager()
    : m_currentProject(nullptr)
{
    loadRecentProjects();
}

ProjectManager::~ProjectManager()
{
    if (m_currentProject) {
        delete m_currentProject;
    }
}

Models::Project* ProjectManager::createProject(const QString& name, const QString& basePath)
{
    QString projectPath = QDir(basePath).filePath(name);

    if (m_currentProject) {
        closeProject();
    }

    m_currentProject = new Models::Project(name, projectPath);

    if (!m_currentProject->createDirectoryStructure()) {
        m_lastError = "Failed to create project directory structure";
        delete m_currentProject;
        m_currentProject = nullptr;
        return nullptr;
    }

    if (!DatabaseManager::instance().saveProject(*m_currentProject)) {
        m_lastError = DatabaseManager::instance().lastError();
        delete m_currentProject;
        m_currentProject = nullptr;
        return nullptr;
    }

    addRecentProject(projectPath);
    emit projectOpened(m_currentProject);

    return m_currentProject;
}

bool ProjectManager::openProject(const QString& projectPath)
{
    if (m_currentProject) {
        closeProject();
    }

    // Load project from database or create new
    m_currentProject = new Models::Project();
    // TODO: Load from database

    addRecentProject(projectPath);
    emit projectOpened(m_currentProject);

    return true;
}

bool ProjectManager::saveProject()
{
    if (!m_currentProject) {
        m_lastError = "No project open";
        return false;
    }

    if (!DatabaseManager::instance().saveProject(*m_currentProject)) {
        m_lastError = DatabaseManager::instance().lastError();
        return false;
    }

    emit projectSaved();
    return true;
}

bool ProjectManager::closeProject()
{
    if (m_currentProject) {
        saveProject();
        delete m_currentProject;
        m_currentProject = nullptr;
        emit projectClosed();
    }

    return true;
}

void ProjectManager::addRecentProject(const QString& projectPath)
{
    m_recentProjects.removeAll(projectPath);
    m_recentProjects.prepend(projectPath);

    if (m_recentProjects.count() > 10) {
        m_recentProjects.removeLast();
    }

    saveRecentProjects();
}

void ProjectManager::loadRecentProjects()
{
    // Load from settings
    // TODO: Implement
}

void ProjectManager::saveRecentProjects()
{
    // Save to settings
    // TODO: Implement
}

} // namespace Core
} // namespace DroneMapper
