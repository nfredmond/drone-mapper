#include "Project.h"
#include <QUuid>
#include <QDir>
#include <QFileInfo>

namespace DroneMapper {
namespace Models {

Project::Project()
    : m_id(generateId())
    , m_name("New Project")
    , m_status(Status::Planning)
    , m_createdDate(QDateTime::currentDateTime())
    , m_modifiedDate(QDateTime::currentDateTime())
    , m_imageCount(0)
{
}

Project::Project(const QString& name, const QString& path)
    : m_id(generateId())
    , m_name(name)
    , m_projectPath(path)
    , m_status(Status::Planning)
    , m_createdDate(QDateTime::currentDateTime())
    , m_modifiedDate(QDateTime::currentDateTime())
    , m_imageCount(0)
{
}

void Project::setName(const QString& name)
{
    m_name = name;
    updateModifiedDate();
}

void Project::setStatus(Status status)
{
    m_status = status;
    updateModifiedDate();
}

void Project::addFlightPlan(const FlightPlan& plan)
{
    m_flightPlans.append(plan);
    updateModifiedDate();
}

void Project::removeFlightPlan(const QString& planId)
{
    for (int i = 0; i < m_flightPlans.count(); ++i) {
        if (m_flightPlans[i].id() == planId) {
            m_flightPlans.removeAt(i);
            updateModifiedDate();
            break;
        }
    }
}

FlightPlan* Project::getFlightPlan(const QString& planId)
{
    for (int i = 0; i < m_flightPlans.count(); ++i) {
        if (m_flightPlans[i].id() == planId) {
            return &m_flightPlans[i];
        }
    }
    return nullptr;
}

QString Project::flightPlansPath() const
{
    return QDir(m_projectPath).filePath("FlightPlans");
}

QString Project::imagesPath() const
{
    return QDir(m_projectPath).filePath("Images");
}

QString Project::processingPath() const
{
    return QDir(m_projectPath).filePath("Processing");
}

QString Project::outputsPath() const
{
    return QDir(m_projectPath).filePath("Outputs");
}

QString Project::reportsPath() const
{
    return QDir(m_projectPath).filePath("Reports");
}

QStringList Project::imagePaths() const
{
    QStringList images;
    QDir imgDir(imagesPath());

    if (imgDir.exists()) {
        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
                << "*.png" << "*.PNG" << "*.tif" << "*.tiff";

        images = imgDir.entryList(filters, QDir::Files);
        for (QString& img : images) {
            img = imgDir.filePath(img);
        }
    }

    return images;
}

bool Project::isValid() const
{
    return !m_name.isEmpty() && !m_projectPath.isEmpty();
}

bool Project::exists() const
{
    return QDir(m_projectPath).exists();
}

bool Project::createDirectoryStructure()
{
    QDir baseDir(m_projectPath);

    if (!baseDir.exists()) {
        if (!baseDir.mkpath(".")) {
            return false;
        }
    }

    // Create subdirectories
    QStringList subdirs = {
        "FlightPlans",
        "Images",
        "Processing",
        "Outputs",
        "Reports"
    };

    for (const QString& subdir : subdirs) {
        if (!baseDir.mkpath(subdir)) {
            return false;
        }
    }

    return true;
}

QString Project::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void Project::updateModifiedDate()
{
    m_modifiedDate = QDateTime::currentDateTime();
}

} // namespace Models
} // namespace DroneMapper
