#ifndef MISSIONPARAMETERSDIALOG_H
#define MISSIONPARAMETERSDIALOG_H

#include "MissionParameters.h"
#include "FlightPlan.h"
#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>

namespace DroneMapper {
namespace UI {

/**
 * @brief Dialog for configuring mission flight parameters
 *
 * Provides comprehensive configuration interface for:
 * - Flight altitude, speed, and direction
 * - Camera model and overlap settings
 * - Pattern type and finish actions
 * - Advanced mission behaviors
 */
class MissionParametersDialog : public QDialog {
    Q_OBJECT

public:
    explicit MissionParametersDialog(QWidget *parent = nullptr);
    ~MissionParametersDialog();

    // Get/set parameters
    Models::MissionParameters parameters() const;
    void setParameters(const Models::MissionParameters& params);

    // Pattern type
    Models::FlightPlan::PatternType patternType() const { return m_patternType; }
    void setPatternType(Models::FlightPlan::PatternType type);

    // Area information (for calculations)
    void setSurveyAreaSize(double squareMeters);
    double estimatedFlightTime() const; // minutes
    double estimatedDistance() const; // meters
    int estimatedPhotoCount() const;

signals:
    void parametersChanged();

private slots:
    void onCameraModelChanged(int index);
    void onAltitudeChanged(double value);
    void onSpeedChanged(double value);
    void onOverlapChanged(int value);
    void onPresetSelected();
    void updateEstimates();
    void loadDefaults();
    void loadPreset(const QString& presetName);

private:
    void setupUI();
    QWidget* createFlightTab();
    QWidget* createCameraTab();
    QWidget* createAdvancedTab();
    QWidget* createEstimatesTab();
    void connectSignals();
    void updateCameraInfo();
    void calculateEstimates();

    // Tabs
    QTabWidget *m_tabs;

    // Flight parameters controls
    QDoubleSpinBox *m_altitudeSpinBox;
    QDoubleSpinBox *m_speedSpinBox;
    QDoubleSpinBox *m_maxSpeedSpinBox;
    QDoubleSpinBox *m_takeoffAltitudeSpinBox;
    QDoubleSpinBox *m_flightDirectionSpinBox;
    QComboBox *m_patternTypeCombo;
    QComboBox *m_finishActionCombo;
    QCheckBox *m_reversePathCheckBox;

    // Camera parameters controls
    QComboBox *m_cameraModelCombo;
    QSlider *m_frontOverlapSlider;
    QSlider *m_sideOverlapSlider;
    QLabel *m_frontOverlapLabel;
    QLabel *m_sideOverlapLabel;
    QDoubleSpinBox *m_gimbalPitchSpinBox;
    QDoubleSpinBox *m_cameraAngleSpinBox;

    // Camera info display
    QLabel *m_cameraInfoLabel;
    QLabel *m_gsdLabel;
    QLabel *m_footprintLabel;

    // Estimates display
    QLabel *m_estimatedDistanceLabel;
    QLabel *m_estimatedTimeLabel;
    QLabel *m_estimatedPhotosLabel;
    QLabel *m_estimatedBatteriesLabel;
    QLabel *m_surveyAreaLabel;

    // Preset buttons
    QPushButton *m_mappingPresetBtn;
    QPushButton *m_inspectionPresetBtn;
    QPushButton *m_lowAltitudePresetBtn;

    // Data
    Models::MissionParameters m_parameters;
    Models::FlightPlan::PatternType m_patternType;
    double m_surveyAreaSize; // square meters

    // Camera specifications (sensor width in mm, focal length in mm)
    struct CameraSpec {
        QString name;
        double sensorWidth;
        double sensorHeight;
        double focalLength;
        int imageWidth;
        int imageHeight;
    };
    static const QMap<Models::MissionParameters::CameraModel, CameraSpec>& getCameraSpecs();
};

} // namespace UI
} // namespace DroneMapper

#endif // MISSIONPARAMETERSDIALOG_H
