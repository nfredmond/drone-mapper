#include "ProjectDashboard.h"
#include <QGridLayout>
#include <QDateTime>
#include <QFont>

namespace DroneMapper {
namespace UI {

// ProjectStats implementation

ProjectStats::ProjectStats()
    : totalMissions(0)
    , completedMissions(0)
    , totalDistance(0.0)
    , totalFlightTime(0)
    , totalPhotos(0)
    , totalAreaCovered(0.0)
    , batteriesUsed(0)
    , totalCost(0.0)
{
}

// ProjectDashboard implementation

ProjectDashboard::ProjectDashboard(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    
    setWindowTitle("Project Dashboard");
    resize(1000, 700);
}

ProjectDashboard::~ProjectDashboard()
{
}

void ProjectDashboard::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("Project Dashboard", this);
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    m_mainLayout->addWidget(titleLabel);
    
    // Top row: Quick actions and Statistics
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    setupQuickActions();
    setupStatisticsPanel();
    
    topLayout->addWidget(m_quickActionsGroup);
    topLayout->addWidget(m_statsGroup, 1);
    
    m_mainLayout->addLayout(topLayout);
    
    // Bottom row: Recent Activity and Resources
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    
    setupRecentActivity();
    setupResourcePanel();
    
    bottomLayout->addWidget(m_activityGroup, 1);
    bottomLayout->addWidget(m_resourceGroup);
    
    m_mainLayout->addLayout(bottomLayout);
}

void ProjectDashboard::setupQuickActions()
{
    m_quickActionsGroup = new QGroupBox("Quick Actions", this);
    QVBoxLayout *layout = new QVBoxLayout();
    
    m_newMissionButton = new QPushButton("New Mission", this);
    m_newMissionButton->setMinimumHeight(40);
    connect(m_newMissionButton, &QPushButton::clicked, this, &ProjectDashboard::onNewMissionClicked);
    
    m_viewRecentButton = new QPushButton("View Recent", this);
    m_viewRecentButton->setMinimumHeight(40);
    connect(m_viewRecentButton, &QPushButton::clicked, this, &ProjectDashboard::onViewRecentClicked);
    
    m_generateReportButton = new QPushButton("Generate Report", this);
    m_generateReportButton->setMinimumHeight(40);
    connect(m_generateReportButton, &QPushButton::clicked, this, &ProjectDashboard::onGenerateReportClicked);
    
    m_refreshButton = new QPushButton("Refresh", this);
    m_refreshButton->setMinimumHeight(40);
    connect(m_refreshButton, &QPushButton::clicked, this, &ProjectDashboard::onRefreshClicked);
    
    layout->addWidget(m_newMissionButton);
    layout->addWidget(m_viewRecentButton);
    layout->addWidget(m_generateReportButton);
    layout->addWidget(m_refreshButton);
    layout->addStretch();
    
    m_quickActionsGroup->setLayout(layout);
}

void ProjectDashboard::setupStatisticsPanel()
{
    m_statsGroup = new QGroupBox("Project Statistics", this);
    QGridLayout *layout = new QGridLayout();
    
    m_totalMissionsLabel = new QLabel("Total Missions: 0", this);
    m_completedMissionsLabel = new QLabel("Completed: 0", this);
    m_totalDistanceLabel = new QLabel("Total Distance: 0 km", this);
    m_totalFlightTimeLabel = new QLabel("Total Flight Time: 0h 0m", this);
    m_totalPhotosLabel = new QLabel("Total Photos: 0", this);
    m_totalAreaLabel = new QLabel("Area Covered: 0 ha", this);
    m_batteriesLabel = new QLabel("Batteries Used: 0", this);
    m_costLabel = new QLabel("Estimated Cost: $0", this);
    
    m_completionProgress = new QProgressBar(this);
    m_completionProgress->setMinimum(0);
    m_completionProgress->setMaximum(100);
    m_completionProgress->setValue(0);
    m_completionProgress->setFormat("Completion: %p%");
    
    layout->addWidget(m_totalMissionsLabel, 0, 0);
    layout->addWidget(m_completedMissionsLabel, 0, 1);
    layout->addWidget(m_totalDistanceLabel, 1, 0);
    layout->addWidget(m_totalFlightTimeLabel, 1, 1);
    layout->addWidget(m_totalPhotosLabel, 2, 0);
    layout->addWidget(m_totalAreaLabel, 2, 1);
    layout->addWidget(m_batteriesLabel, 3, 0);
    layout->addWidget(m_costLabel, 3, 1);
    layout->addWidget(m_completionProgress, 4, 0, 1, 2);
    
    m_statsGroup->setLayout(layout);
}

void ProjectDashboard::setupRecentActivity()
{
    m_activityGroup = new QGroupBox("Recent Activity", this);
    QVBoxLayout *layout = new QVBoxLayout();
    
    m_activityList = new QListWidget(this);
    connect(m_activityList, &QListWidget::itemDoubleClicked,
            this, &ProjectDashboard::onMissionItemDoubleClicked);
    
    layout->addWidget(m_activityList);
    m_activityGroup->setLayout(layout);
    
    // Add some placeholder activity
    m_activityList->addItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm") + " - Dashboard opened");
}

void ProjectDashboard::setupResourcePanel()
{
    m_resourceGroup = new QGroupBox("Resource Usage", this);
    QVBoxLayout *layout = new QVBoxLayout();
    
    m_storageUsedLabel = new QLabel("Storage: 0 GB", this);
    m_imagesProcessedLabel = new QLabel("Images: 0", this);
    m_avgQualityLabel = new QLabel("Avg Quality: N/A", this);
    
    layout->addWidget(m_storageUsedLabel);
    layout->addWidget(m_imagesProcessedLabel);
    layout->addWidget(m_avgQualityLabel);
    layout->addStretch();
    
    m_resourceGroup->setLayout(layout);
}

void ProjectDashboard::refreshData()
{
    // Refresh dashboard with current data
    // In a real implementation, this would query the database
    
    m_activityList->addItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm") + " - Dashboard refreshed");
}

void ProjectDashboard::addMission(const QString& mission, bool completed)
{
    QString status = completed ? "[COMPLETED]" : "[PLANNED]";
    QString item = QString("%1 %2 - %3")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"))
        .arg(status)
        .arg(mission);
    
    m_activityList->insertItem(0, item);
    
    // Update stats
    m_stats.totalMissions++;
    if (completed) {
        m_stats.completedMissions++;
    }
    
    updateStatistics(m_stats);
}

void ProjectDashboard::updateStatistics(const ProjectStats& stats)
{
    m_stats = stats;
    
    m_totalMissionsLabel->setText(QString("Total Missions: %1").arg(stats.totalMissions));
    m_completedMissionsLabel->setText(QString("Completed: %1").arg(stats.completedMissions));
    m_totalDistanceLabel->setText(QString("Total Distance: %1").arg(formatDistance(stats.totalDistance)));
    m_totalFlightTimeLabel->setText(QString("Total Flight Time: %1").arg(formatTime(stats.totalFlightTime)));
    m_totalPhotosLabel->setText(QString("Total Photos: %1").arg(stats.totalPhotos));
    m_totalAreaLabel->setText(QString("Area Covered: %1").arg(formatArea(stats.totalAreaCovered)));
    m_batteriesLabel->setText(QString("Batteries Used: %1").arg(stats.batteriesUsed));
    m_costLabel->setText(QString("Estimated Cost: $%1").arg(stats.totalCost, 0, 'f', 2));
    
    // Update completion progress
    if (stats.totalMissions > 0) {
        int percentage = (stats.completedMissions * 100) / stats.totalMissions;
        m_completionProgress->setValue(percentage);
    } else {
        m_completionProgress->setValue(0);
    }
}

void ProjectDashboard::onNewMissionClicked()
{
    emit newMissionRequested();
}

void ProjectDashboard::onViewRecentClicked()
{
    if (m_activityList->count() > 0) {
        QListWidgetItem *item = m_activityList->item(0);
        if (item) {
            onMissionItemDoubleClicked(item);
        }
    }
}

void ProjectDashboard::onGenerateReportClicked()
{
    emit generateReportRequested();
}

void ProjectDashboard::onRefreshClicked()
{
    refreshData();
}

void ProjectDashboard::onMissionItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    
    QString text = item->text();
    // Extract mission name (simplified)
    QStringList parts = text.split(" - ");
    if (parts.size() >= 2) {
        emit viewMissionRequested(parts.last());
    }
}

QString ProjectDashboard::formatTime(int seconds)
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    return QString("%1h %2m").arg(hours).arg(minutes);
}

QString ProjectDashboard::formatDistance(double meters)
{
    if (meters >= 1000.0) {
        return QString("%1 km").arg(meters / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 m").arg(meters, 0, 'f', 0);
    }
}

QString ProjectDashboard::formatArea(double squareMeters)
{
    double hectares = squareMeters / 10000.0;
    return QString("%1 ha").arg(hectares, 0, 'f', 2);
}

} // namespace UI
} // namespace DroneMapper
