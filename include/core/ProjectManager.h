#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "Project.h"
#include <QString>
#include <QObject>
#include <QList>

namespace DroneMapper {
namespace Core {

/**
 * @brief Manages project lifecycle and operations
 */
class ProjectManager : public QObject {
    Q_OBJECT

public:
    static ProjectManager& instance();

    // Project operations
    Models::Project* createProject(const QString& name, const QString& basePath);
    bool openProject(const QString& projectPath);
    bool saveProject();
    bool closeProject();

    Models::Project* currentProject() { return m_currentProject; }
    const Models::Project* currentProject() const { return m_currentProject; }

    // Recent projects
    QStringList recentProjects() const { return m_recentProjects; }
    void addRecentProject(const QString& projectPath);

    QString lastError() const { return m_lastError; }

signals:
    void projectOpened(Models::Project* project);
    void projectClosed();
    void projectSaved();
    void projectModified();

private:
    ProjectManager();
    ~ProjectManager();
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;

    void loadRecentProjects();
    void saveRecentProjects();

    Models::Project* m_currentProject;
    QStringList m_recentProjects;
    QString m_lastError;
};

} // namespace Core
} // namespace DroneMapper

#endif // PROJECTMANAGER_H
