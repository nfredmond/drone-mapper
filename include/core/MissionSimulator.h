#ifndef MISSIONSIMULATOR_H
#define MISSIONSIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QVector>
#include "models/FlightPlan.h"
#include "models/GeospatialCoordinate.h"

namespace DroneMapper {
namespace Core {

/**
 * @brief Simulation state data
 */
struct SimulationState {
    int currentWaypointIndex;
    Models::GeospatialCoordinate currentPosition;
    double currentAltitude;
    double currentSpeed;
    double currentBatteryPercent;
    int elapsedTime;  // seconds
    int photosTaken;
    double distanceTraveled;  // meters
    bool isComplete;

    SimulationState();
};

/**
 * @brief Simulation statistics
 */
struct SimulationStatistics {
    int totalWaypoints;
    double totalDistance;  // meters
    int totalFlightTime;  // seconds
    int totalPhotoCount;
    double surveyAreaCovered;  // square meters
    int batteryChangesRequired;
    double maxAltitude;  // meters
    double avgSpeed;  // m/s
    QVector<double> batteryPercentages;  // battery % at each waypoint
    QVector<QDateTime> waypointTimes;  // estimated time at each waypoint

    SimulationStatistics();
};

/**
 * @brief Mission Simulator & Preview
 *
 * Provides real-time mission simulation capabilities:
 * - Waypoint-by-waypoint flight simulation
 * - Battery consumption modeling
 * - Photo capture timing
 * - Flight time estimation
 * - Speed and altitude profiling
 * - Mission validation and warnings
 * - Playback controls (play, pause, speed adjustment)
 *
 * Usage:
 *   MissionSimulator simulator;
 *   simulator.loadFlightPlan(plan);
 *   simulator.setSimulationSpeed(2.0);  // 2x speed
 *   simulator.start();
 *
 *   connect(&simulator, &MissionSimulator::stateUpdated,
 *           this, &MyWidget::onSimulationUpdate);
 */
class MissionSimulator : public QObject {
    Q_OBJECT

public:
    explicit MissionSimulator(QObject *parent = nullptr);
    ~MissionSimulator();

    /**
     * @brief Load flight plan for simulation
     * @param plan Flight plan to simulate
     * @return True if plan is valid
     */
    bool loadFlightPlan(const Models::FlightPlan& plan);

    /**
     * @brief Start simulation
     */
    void start();

    /**
     * @brief Pause simulation
     */
    void pause();

    /**
     * @brief Stop and reset simulation
     */
    void stop();

    /**
     * @brief Reset to beginning
     */
    void reset();

    /**
     * @brief Set simulation speed multiplier
     * @param speed Speed multiplier (1.0 = real-time, 2.0 = 2x, etc.)
     */
    void setSimulationSpeed(double speed);

    /**
     * @brief Jump to specific waypoint
     * @param waypointIndex Waypoint index
     */
    void jumpToWaypoint(int waypointIndex);

    /**
     * @brief Get current simulation state
     * @return Current state
     */
    SimulationState currentState() const { return m_currentState; }

    /**
     * @brief Get simulation statistics
     * @return Statistics
     */
    SimulationStatistics statistics() const { return m_statistics; }

    /**
     * @brief Check if simulation is running
     * @return True if running
     */
    bool isRunning() const { return m_isRunning; }

    /**
     * @brief Get simulation progress (0.0 to 1.0)
     * @return Progress percentage
     */
    double progress() const;

    /**
     * @brief Validate mission for safety issues
     * @return List of warning messages
     */
    QStringList validateMission() const;

signals:
    /**
     * @brief Emitted when simulation state updates
     * @param state Current state
     */
    void stateUpdated(const SimulationState& state);

    /**
     * @brief Emitted when waypoint is reached
     * @param waypointIndex Waypoint index
     */
    void waypointReached(int waypointIndex);

    /**
     * @brief Emitted when photo is captured
     * @param photoNumber Photo number
     * @param position Photo position
     */
    void photoCaptured(int photoNumber, const Models::GeospatialCoordinate& position);

    /**
     * @brief Emitted when battery change is needed
     * @param batteryNumber Battery number
     */
    void batteryChangeRequired(int batteryNumber);

    /**
     * @brief Emitted when simulation completes
     * @param statistics Final statistics
     */
    void simulationCompleted(const SimulationStatistics& statistics);

    /**
     * @brief Emitted on validation warning
     * @param message Warning message
     */
    void warning(const QString& message);

private slots:
    void updateSimulation();

private:
    Models::FlightPlan m_flightPlan;
    SimulationState m_currentState;
    SimulationStatistics m_statistics;
    QTimer *m_updateTimer;
    double m_simulationSpeed;
    bool m_isRunning;

    // Simulation parameters
    static constexpr double DEFAULT_CRUISE_SPEED = 10.0;  // m/s
    static constexpr double BATTERY_CAPACITY_MAH = 5000.0;
    static constexpr double FLIGHT_CURRENT_MA = 10000.0;  // 10A average
    static constexpr double PHOTO_INTERVAL = 2.0;  // seconds
    static constexpr int UPDATE_INTERVAL_MS = 100;  // 100ms updates

    void calculateStatistics();
    void interpolatePosition(double t);
    double calculateBatteryUsage(double flightTime) const;
    int estimatePhotoCount(double segmentTime) const;
    QStringList performSafetyChecks() const;
};

} // namespace Core
} // namespace DroneMapper

#endif // MISSIONSIMULATOR_H
