#ifndef SIMULATIONPREVIEWWIDGET_H
#define SIMULATIONPREVIEWWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "core/MissionSimulator.h"
#include "models/FlightPlan.h"

namespace DroneMapper {
namespace UI {

/**
 * @brief Simulation Preview Widget
 *
 * Provides comprehensive mission simulation preview with:
 * - Real-time 3D flight path visualization
 * - Playback controls (play/pause/stop/speed)
 * - Live statistics display
 * - Battery monitoring
 * - Photo capture tracking
 * - Validation warnings
 * - Progress tracking
 *
 * Usage:
 *   SimulationPreviewWidget *preview = new SimulationPreviewWidget(this);
 *   preview->loadFlightPlan(flightPlan);
 *   preview->show();
 */
class SimulationPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit SimulationPreviewWidget(QWidget *parent = nullptr);
    ~SimulationPreviewWidget();

    /**
     * @brief Load flight plan for preview
     * @param plan Flight plan
     */
    void loadFlightPlan(const Models::FlightPlan& plan);

    /**
     * @brief Get simulator instance
     * @return Simulator
     */
    Core::MissionSimulator* simulator() { return m_simulator; }

signals:
    /**
     * @brief Emitted when simulation completes
     */
    void simulationFinished();

    /**
     * @brief Emitted when user closes widget
     */
    void closed();

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onSpeedChanged(int value);
    void onSimulationStateUpdated(const Core::SimulationState& state);
    void onWaypointReached(int waypointIndex);
    void onPhotoCaptured(int photoNumber, const Models::GeospatialCoordinate& position);
    void onBatteryChangeRequired(int batteryNumber);
    void onSimulationCompleted(const Core::SimulationStatistics& statistics);
    void onWarning(const QString& message);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void updateStatisticsDisplay();
    void updateStateDisplay(const Core::SimulationState& state);
    void drawFlightPath();
    QString formatTime(int seconds) const;
    QString formatDistance(double meters) const;

    // Core components
    Core::MissionSimulator *m_simulator;
    Models::FlightPlan m_flightPlan;

    // UI Components
    QVBoxLayout *m_mainLayout;

    // Visualization area
    QLabel *m_visualizationLabel;
    QPixmap m_flightPathPixmap;

    // Control panel
    QGroupBox *m_controlGroup;
    QPushButton *m_playPauseButton;
    QPushButton *m_stopButton;
    QSlider *m_speedSlider;
    QLabel *m_speedLabel;
    QProgressBar *m_progressBar;

    // Statistics panel
    QGroupBox *m_statsGroup;
    QLabel *m_totalDistanceLabel;
    QLabel *m_totalTimeLabel;
    QLabel *m_totalPhotosLabel;
    QLabel *m_batteryCountLabel;
    QLabel *m_surveyAreaLabel;

    // Current state panel
    QGroupBox *m_stateGroup;
    QLabel *m_currentWaypointLabel;
    QLabel *m_currentPositionLabel;
    QLabel *m_currentAltitudeLabel;
    QLabel *m_currentSpeedLabel;
    QLabel *m_currentBatteryLabel;
    QLabel *m_photosTakenLabel;
    QLabel *m_elapsedTimeLabel;
    QProgressBar *m_batteryBar;

    // Warnings panel
    QGroupBox *m_warningsGroup;
    QTextEdit *m_warningsText;

    // State
    bool m_isPlaying;
};

} // namespace UI
} // namespace DroneMapper

#endif // SIMULATIONPREVIEWWIDGET_H
