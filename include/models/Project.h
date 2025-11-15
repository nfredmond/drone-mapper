#ifndef PROJECT_H
#define PROJECT_H

#include "FlightPlan.h"
#include <QString>
#include <QDateTime>
#include <QList>
#include <QFileInfo>

namespace DroneMapper {
namespace Models {

/**
 * @brief Represents a drone mapping project containing flight plans and processing results
 */
class Project {
public:
    enum class Status {
        Planning,
        ReadyToFly,
        DataCollected,
        Processing,
        Completed
    };

    Project();
    Project(const QString& name, const QString& path);

    // Basic properties
    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    QString projectPath() const { return m_projectPath; }
    Status status() const { return m_status; }
    QDateTime createdDate() const { return m_createdDate; }
    QDateTime modifiedDate() const { return m_modifiedDate; }

    void setName(const QString& name);
    void setDescription(const QString& desc) { m_description = desc; }
    void setStatus(Status status);

    // Flight plans
    QList<FlightPlan> flightPlans() const { return m_flightPlans; }
    void addFlightPlan(const FlightPlan& plan);
    void removeFlightPlan(const QString& planId);
    FlightPlan* getFlightPlan(const QString& planId);

    // Project structure
    QString flightPlansPath() const;
    QString imagesPath() const;
    QString processingPath() const;
    QString outputsPath() const;
    QString reportsPath() const;

    // Image management
    int imageCount() const { return m_imageCount; }
    void setImageCount(int count) { m_imageCount = count; }
    QStringList imagePaths() const;

    // Validation
    bool isValid() const;
    bool exists() const;
    bool createDirectoryStructure();

private:
    QString m_id;
    QString m_name;
    QString m_description;
    QString m_projectPath;
    Status m_status;
    QDateTime m_createdDate;
    QDateTime m_modifiedDate;

    QList<FlightPlan> m_flightPlans;
    int m_imageCount;

    QString generateId() const;
    void updateModifiedDate();
};

} // namespace Models
} // namespace DroneMapper

#endif // PROJECT_H
