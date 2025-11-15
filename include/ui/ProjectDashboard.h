#ifndef PROJECTDASHBOARD_H
#define PROJECTDASHBOARD_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QProgressBar>
#include "models/FlightPlan.h"

namespace DroneMapper {
namespace UI {

/**
 * @brief Project statistics data
 */
struct ProjectStats {
    int totalMissions;
    int completedMissions;
    double totalDistance;        // meters
    int totalFlightTime;        // seconds
    int totalPhotos;
    double totalAreaCovered;    // square meters
    int batteriesUsed;
    double totalCost;           // estimated cost

    ProjectStats();
};

/**
 * @brief Project Dashboard Widget
 *
 * Comprehensive project overview and analytics dashboard:
 * - Mission summary statistics
 * - Progress tracking
 * - Cost analysis
 * - Recent activity timeline
 * - Quick actions panel
 * - Resource usage charts
 *
 * Usage:
 *   ProjectDashboard *dashboard = new ProjectDashboard(this);
 *   dashboard->show();
 */
class ProjectDashboard : public QWidget {
    Q_OBJECT

public:
    explicit ProjectDashboard(QWidget *parent = nullptr);
    ~ProjectDashboard();

    /**
     * @brief Update dashboard with current project data
     */
    void refreshData();

    /**
     * @brief Add mission to dashboard
     * @param mission Mission name
     * @param completed Whether mission is completed
     */
    void addMission(const QString& mission, bool completed = false);

    /**
     * @brief Update statistics
     * @param stats Project statistics
     */
    void updateStatistics(const ProjectStats& stats);

signals:
    /**
     * @brief Emitted when user wants to create new mission
     */
    void newMissionRequested();

    /**
     * @brief Emitted when user wants to view mission
     * @param missionName Mission name
     */
    void viewMissionRequested(const QString& missionName);

    /**
     * @brief Emitted when user wants to generate report
     */
    void generateReportRequested();

private slots:
    void onNewMissionClicked();
    void onViewRecentClicked();
    void onGenerateReportClicked();
    void onRefreshClicked();
    void onMissionItemDoubleClicked(QListWidgetItem *item);

private:
    void setupUI();
    void setupQuickActions();
    void setupStatisticsPanel();
    void setupRecentActivity();
    void setupResourcePanel();
    QString formatTime(int seconds);
    QString formatDistance(double meters);
    QString formatArea(double squareMeters);

    // UI Components
    QVBoxLayout *m_mainLayout;

    // Quick actions
    QGroupBox *m_quickActionsGroup;
    QPushButton *m_newMissionButton;
    QPushButton *m_viewRecentButton;
    QPushButton *m_generateReportButton;
    QPushButton *m_refreshButton;

    // Statistics panel
    QGroupBox *m_statsGroup;
    QLabel *m_totalMissionsLabel;
    QLabel *m_completedMissionsLabel;
    QLabel *m_totalDistanceLabel;
    QLabel *m_totalFlightTimeLabel;
    QLabel *m_totalPhotosLabel;
    QLabel *m_totalAreaLabel;
    QLabel *m_batteriesLabel;
    QLabel *m_costLabel;
    QProgressBar *m_completionProgress;

    // Recent activity
    QGroupBox *m_activityGroup;
    QListWidget *m_activityList;

    // Resource usage
    QGroupBox *m_resourceGroup;
    QLabel *m_storageUsedLabel;
    QLabel *m_imagesProcessedLabel;
    QLabel *m_avgQualityLabel;

    // Data
    ProjectStats m_stats;
};

} // namespace UI
} // namespace DroneMapper

#endif // PROJECTDASHBOARD_H
