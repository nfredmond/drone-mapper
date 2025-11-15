#include "MissionParametersDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <cmath>

namespace DroneMapper {
namespace UI {

// Static camera specifications
const QMap<Models::MissionParameters::CameraModel, MissionParametersDialog::CameraSpec>&
MissionParametersDialog::getCameraSpecs() {
    static QMap<Models::MissionParameters::CameraModel, CameraSpec> specs;
    if (specs.isEmpty()) {
        specs[Models::MissionParameters::CameraModel::DJI_Mini3] = {
            "DJI Mini 3",
            6.4,  // sensor width mm
            4.8,  // sensor height mm
            6.7,  // focal length mm (24mm equivalent)
            4000, // image width px
            3000  // image height px
        };
        specs[Models::MissionParameters::CameraModel::DJI_Mini3Pro] = {
            "DJI Mini 3 Pro",
            9.7,  // sensor width mm (1/1.3" sensor)
            7.3,  // sensor height mm
            6.7,  // focal length mm
            4000,
            3000
        };
        specs[Models::MissionParameters::CameraModel::DJI_Air3] = {
            "DJI Air 3",
            9.7,  // Wide camera
            7.3,
            6.7,
            4000,
            3000
        };
        specs[Models::MissionParameters::CameraModel::DJI_Mavic3] = {
            "DJI Mavic 3",
            17.3, // 4/3" CMOS
            13.0,
            12.29,
            5280,
            3956
        };
        specs[Models::MissionParameters::CameraModel::Custom] = {
            "Custom Camera",
            6.4,
            4.8,
            6.7,
            4000,
            3000
        };
    }
    return specs;
}

MissionParametersDialog::MissionParametersDialog(QWidget *parent)
    : QDialog(parent)
    , m_patternType(Models::FlightPlan::PatternType::Polygon)
    , m_surveyAreaSize(0.0)
{
    setWindowTitle(tr("Mission Parameters"));
    resize(600, 500);

    setupUI();
    connectSignals();
    loadDefaults();
}

MissionParametersDialog::~MissionParametersDialog()
{
}

void MissionParametersDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create tabs
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createFlightTab(), tr("Flight"));
    m_tabs->addTab(createCameraTab(), tr("Camera"));
    m_tabs->addTab(createAdvancedTab(), tr("Advanced"));
    m_tabs->addTab(createEstimatesTab(), tr("Estimates"));

    mainLayout->addWidget(m_tabs);

    // Preset buttons
    QHBoxLayout *presetsLayout = new QHBoxLayout();
    presetsLayout->addWidget(new QLabel(tr("Quick Presets:"), this));

    m_mappingPresetBtn = new QPushButton(tr("Mapping (75% overlap)"), this);
    m_inspectionPresetBtn = new QPushButton(tr("Inspection (Low altitude)"), this);
    m_lowAltitudePresetBtn = new QPushButton(tr("High Detail (85% overlap)"), this);

    presetsLayout->addWidget(m_mappingPresetBtn);
    presetsLayout->addWidget(m_inspectionPresetBtn);
    presetsLayout->addWidget(m_lowAltitudePresetBtn);
    presetsLayout->addStretch();

    mainLayout->addLayout(presetsLayout);

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

QWidget* MissionParametersDialog::createFlightTab()
{
    QWidget *tab = new QWidget();
    QFormLayout *layout = new QFormLayout(tab);

    // Altitude
    m_altitudeSpinBox = new QDoubleSpinBox();
    m_altitudeSpinBox->setRange(10.0, 500.0);
    m_altitudeSpinBox->setSingleStep(5.0);
    m_altitudeSpinBox->setValue(100.0);
    m_altitudeSpinBox->setSuffix(" m");
    layout->addRow(tr("Flight Altitude:"), m_altitudeSpinBox);

    // Speed
    m_speedSpinBox = new QDoubleSpinBox();
    m_speedSpinBox->setRange(1.0, 20.0);
    m_speedSpinBox->setSingleStep(0.5);
    m_speedSpinBox->setValue(10.0);
    m_speedSpinBox->setSuffix(" m/s");
    layout->addRow(tr("Flight Speed:"), m_speedSpinBox);

    // Pattern type
    m_patternTypeCombo = new QComboBox();
    m_patternTypeCombo->addItem(tr("Manual Waypoints"),
        static_cast<int>(Models::FlightPlan::PatternType::Manual));
    m_patternTypeCombo->addItem(tr("Polygon Coverage"),
        static_cast<int>(Models::FlightPlan::PatternType::Polygon));
    m_patternTypeCombo->addItem(tr("Grid Pattern"),
        static_cast<int>(Models::FlightPlan::PatternType::Grid));
    m_patternTypeCombo->addItem(tr("Circular Pattern"),
        static_cast<int>(Models::FlightPlan::PatternType::Circular));
    m_patternTypeCombo->addItem(tr("Corridor Mapping"),
        static_cast<int>(Models::FlightPlan::PatternType::Corridor));
    m_patternTypeCombo->setCurrentIndex(1); // Polygon default
    layout->addRow(tr("Pattern Type:"), m_patternTypeCombo);

    // Flight direction
    m_flightDirectionSpinBox = new QDoubleSpinBox();
    m_flightDirectionSpinBox->setRange(0.0, 360.0);
    m_flightDirectionSpinBox->setSingleStep(15.0);
    m_flightDirectionSpinBox->setValue(0.0);
    m_flightDirectionSpinBox->setSuffix("°");
    m_flightDirectionSpinBox->setWrapping(true);
    layout->addRow(tr("Flight Direction:"), m_flightDirectionSpinBox);

    // Reverse path
    m_reversePathCheckBox = new QCheckBox(tr("Reverse flight path direction"));
    layout->addRow("", m_reversePathCheckBox);

    // Finish action
    m_finishActionCombo = new QComboBox();
    m_finishActionCombo->addItem(tr("Return to Home"),
        static_cast<int>(Models::MissionParameters::FinishAction::ReturnToHome));
    m_finishActionCombo->addItem(tr("Hover"),
        static_cast<int>(Models::MissionParameters::FinishAction::Hover));
    m_finishActionCombo->addItem(tr("Land"),
        static_cast<int>(Models::MissionParameters::FinishAction::Land));
    m_finishActionCombo->addItem(tr("Go to First Waypoint"),
        static_cast<int>(Models::MissionParameters::FinishAction::GoToFirstWaypoint));
    layout->addRow(tr("Finish Action:"), m_finishActionCombo);

    return tab;
}

QWidget* MissionParametersDialog::createCameraTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    QFormLayout *layout = new QFormLayout();

    // Camera model
    m_cameraModelCombo = new QComboBox();
    m_cameraModelCombo->addItem(tr("DJI Mini 3"),
        static_cast<int>(Models::MissionParameters::CameraModel::DJI_Mini3));
    m_cameraModelCombo->addItem(tr("DJI Mini 3 Pro"),
        static_cast<int>(Models::MissionParameters::CameraModel::DJI_Mini3Pro));
    m_cameraModelCombo->addItem(tr("DJI Air 3"),
        static_cast<int>(Models::MissionParameters::CameraModel::DJI_Air3));
    m_cameraModelCombo->addItem(tr("DJI Mavic 3"),
        static_cast<int>(Models::MissionParameters::CameraModel::DJI_Mavic3));
    m_cameraModelCombo->addItem(tr("Custom Camera"),
        static_cast<int>(Models::MissionParameters::CameraModel::Custom));
    layout->addRow(tr("Camera Model:"), m_cameraModelCombo);

    // Front overlap
    QHBoxLayout *frontOverlapLayout = new QHBoxLayout();
    m_frontOverlapSlider = new QSlider(Qt::Horizontal);
    m_frontOverlapSlider->setRange(50, 95);
    m_frontOverlapSlider->setValue(75);
    m_frontOverlapLabel = new QLabel("75%");
    m_frontOverlapLabel->setMinimumWidth(40);
    frontOverlapLayout->addWidget(m_frontOverlapSlider);
    frontOverlapLayout->addWidget(m_frontOverlapLabel);
    layout->addRow(tr("Front Overlap:"), frontOverlapLayout);

    // Side overlap
    QHBoxLayout *sideOverlapLayout = new QHBoxLayout();
    m_sideOverlapSlider = new QSlider(Qt::Horizontal);
    m_sideOverlapSlider->setRange(50, 95);
    m_sideOverlapSlider->setValue(75);
    m_sideOverlapLabel = new QLabel("75%");
    m_sideOverlapLabel->setMinimumWidth(40);
    sideOverlapLayout->addWidget(m_sideOverlapSlider);
    sideOverlapLayout->addWidget(m_sideOverlapLabel);
    layout->addRow(tr("Side Overlap:"), sideOverlapLayout);

    // Gimbal pitch
    m_gimbalPitchSpinBox = new QDoubleSpinBox();
    m_gimbalPitchSpinBox->setRange(-90.0, 0.0);
    m_gimbalPitchSpinBox->setSingleStep(5.0);
    m_gimbalPitchSpinBox->setValue(-90.0);
    m_gimbalPitchSpinBox->setSuffix("° (0° = forward, -90° = nadir)");
    layout->addRow(tr("Gimbal Pitch:"), m_gimbalPitchSpinBox);

    // Camera angle
    m_cameraAngleSpinBox = new QDoubleSpinBox();
    m_cameraAngleSpinBox->setRange(-45.0, 45.0);
    m_cameraAngleSpinBox->setSingleStep(5.0);
    m_cameraAngleSpinBox->setValue(0.0);
    m_cameraAngleSpinBox->setSuffix("°");
    layout->addRow(tr("Camera Angle:"), m_cameraAngleSpinBox);

    mainLayout->addLayout(layout);

    // Camera info group
    QGroupBox *infoGroup = new QGroupBox(tr("Camera Information"));
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    m_cameraInfoLabel = new QLabel(tr("Select camera model to view specifications"));
    m_cameraInfoLabel->setWordWrap(true);
    m_gsdLabel = new QLabel(tr("GSD: - cm/pixel"));
    m_footprintLabel = new QLabel(tr("Footprint: - x - m"));

    infoLayout->addWidget(m_cameraInfoLabel);
    infoLayout->addWidget(m_gsdLabel);
    infoLayout->addWidget(m_footprintLabel);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    return tab;
}

QWidget* MissionParametersDialog::createAdvancedTab()
{
    QWidget *tab = new QWidget();
    QFormLayout *layout = new QFormLayout(tab);

    // Takeoff altitude
    m_takeoffAltitudeSpinBox = new QDoubleSpinBox();
    m_takeoffAltitudeSpinBox->setRange(1.0, 50.0);
    m_takeoffAltitudeSpinBox->setSingleStep(1.0);
    m_takeoffAltitudeSpinBox->setValue(10.0);
    m_takeoffAltitudeSpinBox->setSuffix(" m");
    layout->addRow(tr("Takeoff Altitude:"), m_takeoffAltitudeSpinBox);

    // Max speed
    m_maxSpeedSpinBox = new QDoubleSpinBox();
    m_maxSpeedSpinBox->setRange(1.0, 25.0);
    m_maxSpeedSpinBox->setSingleStep(0.5);
    m_maxSpeedSpinBox->setValue(15.0);
    m_maxSpeedSpinBox->setSuffix(" m/s");
    layout->addRow(tr("Maximum Speed:"), m_maxSpeedSpinBox);

    layout->addRow(new QLabel(tr("\nNote: Advanced settings affect mission behavior.\n"
                                  "Modify only if you understand the implications."), this));

    return tab;
}

QWidget* MissionParametersDialog::createEstimatesTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    QGroupBox *estimatesGroup = new QGroupBox(tr("Mission Estimates"));
    QFormLayout *layout = new QFormLayout(estimatesGroup);

    m_surveyAreaLabel = new QLabel(tr("Not calculated"));
    m_estimatedDistanceLabel = new QLabel(tr("Not calculated"));
    m_estimatedTimeLabel = new QLabel(tr("Not calculated"));
    m_estimatedPhotosLabel = new QLabel(tr("Not calculated"));
    m_estimatedBatteriesLabel = new QLabel(tr("Not calculated"));

    QFont boldFont;
    boldFont.setBold(true);

    layout->addRow(tr("Survey Area:"), m_surveyAreaLabel);
    layout->addRow(tr("Flight Distance:"), m_estimatedDistanceLabel);
    layout->addRow(tr("Flight Time:"), m_estimatedTimeLabel);
    layout->addRow(tr("Estimated Photos:"), m_estimatedPhotosLabel);
    layout->addRow(tr("Batteries Needed:"), m_estimatedBatteriesLabel);

    mainLayout->addWidget(estimatesGroup);

    // Info label
    QLabel *infoLabel = new QLabel(
        tr("<b>Battery Assumptions:</b><br>"
           "• DJI Mini 3: ~25 min flight time<br>"
           "• DJI Mini 3 Pro: ~30 min flight time<br>"
           "• DJI Air 3: ~40 min flight time<br>"
           "• DJI Mavic 3: ~45 min flight time<br>"
           "<br>"
           "<i>Actual flight time varies with wind, temperature, and flight behavior.</i>"));
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return tab;
}

void MissionParametersDialog::connectSignals()
{
    connect(m_altitudeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MissionParametersDialog::onAltitudeChanged);
    connect(m_speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MissionParametersDialog::onSpeedChanged);
    connect(m_cameraModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MissionParametersDialog::onCameraModelChanged);

    connect(m_frontOverlapSlider, &QSlider::valueChanged, this, [this](int value) {
        m_frontOverlapLabel->setText(QString("%1%").arg(value));
        updateEstimates();
    });

    connect(m_sideOverlapSlider, &QSlider::valueChanged, this, [this](int value) {
        m_sideOverlapLabel->setText(QString("%1%").arg(value));
        updateEstimates();
    });

    // Preset buttons
    connect(m_mappingPresetBtn, &QPushButton::clicked, this, [this]() {
        loadPreset("mapping");
    });
    connect(m_inspectionPresetBtn, &QPushButton::clicked, this, [this]() {
        loadPreset("inspection");
    });
    connect(m_lowAltitudePresetBtn, &QPushButton::clicked, this, [this]() {
        loadPreset("highdetail");
    });
}

void MissionParametersDialog::onCameraModelChanged(int index)
{
    updateCameraInfo();
    updateEstimates();
}

void MissionParametersDialog::onAltitudeChanged(double value)
{
    updateCameraInfo();
    updateEstimates();
}

void MissionParametersDialog::onSpeedChanged(double value)
{
    updateEstimates();
}

void MissionParametersDialog::onOverlapChanged(int value)
{
    updateEstimates();
}

void MissionParametersDialog::updateCameraInfo()
{
    auto model = static_cast<Models::MissionParameters::CameraModel>(
        m_cameraModelCombo->currentData().toInt());

    const auto& specs = getCameraSpecs();
    if (!specs.contains(model)) {
        return;
    }

    const CameraSpec& spec = specs[model];

    // Update camera info
    m_cameraInfoLabel->setText(QString(
        "<b>%1</b><br>"
        "Sensor: %2 x %3 mm<br>"
        "Focal Length: %4 mm<br>"
        "Image: %5 x %6 px")
        .arg(spec.name)
        .arg(spec.sensorWidth, 0, 'f', 1)
        .arg(spec.sensorHeight, 0, 'f', 1)
        .arg(spec.focalLength, 0, 'f', 1)
        .arg(spec.imageWidth)
        .arg(spec.imageHeight));

    // Calculate GSD and footprint
    double altitude = m_altitudeSpinBox->value();
    double gsd = (spec.sensorWidth * altitude * 100) / (spec.focalLength * spec.imageWidth);

    double footprintWidth = (spec.sensorWidth * altitude) / spec.focalLength;
    double footprintHeight = (spec.sensorHeight * altitude) / spec.focalLength;

    m_gsdLabel->setText(QString("GSD: %1 cm/pixel")
        .arg(gsd, 0, 'f', 2));
    m_footprintLabel->setText(QString("Footprint: %1 x %2 m")
        .arg(footprintWidth, 0, 'f', 1)
        .arg(footprintHeight, 0, 'f', 1));
}

void MissionParametersDialog::updateEstimates()
{
    calculateEstimates();
    emit parametersChanged();
}

void MissionParametersDialog::calculateEstimates()
{
    if (m_surveyAreaSize <= 0.0) {
        return; // Can't calculate without area
    }

    // Get current parameters
    double altitude = m_altitudeSpinBox->value();
    double speed = m_speedSpinBox->value();
    double frontOverlap = m_frontOverlapSlider->value() / 100.0;
    double sideOverlap = m_sideOverlapSlider->value() / 100.0;

    auto model = static_cast<Models::MissionParameters::CameraModel>(
        m_cameraModelCombo->currentData().toInt());
    const CameraSpec& spec = getCameraSpecs()[model];

    // Calculate footprint and spacing
    double footprintWidth = (spec.sensorWidth * altitude) / spec.focalLength;
    double footprintHeight = (spec.sensorHeight * altitude) / spec.focalLength;

    double lineSpacing = footprintWidth * (1.0 - sideOverlap);
    double photoSpacing = footprintHeight * (1.0 - frontOverlap);

    // Estimate number of lines and photos per line
    double areaSqrtM = std::sqrt(m_surveyAreaSize);
    int numLines = static_cast<int>(std::ceil(areaSqrtM / lineSpacing));
    int photosPerLine = static_cast<int>(std::ceil(areaSqrtM / photoSpacing));

    // Estimate distance
    double lineLength = areaSqrtM;
    double totalDistance = numLines * lineLength;
    totalDistance *= 1.1; // Add 10% for turns

    // Estimate time (in seconds)
    double flightTimeSeconds = totalDistance / speed;
    flightTimeSeconds *= 1.15; // Add 15% for photo capture time

    // Estimate photos
    int totalPhotos = numLines * photosPerLine;

    // Estimate batteries (based on camera model)
    double batteryMinutes = 25.0; // Default to Mini 3
    if (model == Models::MissionParameters::CameraModel::DJI_Mini3Pro) {
        batteryMinutes = 30.0;
    } else if (model == Models::MissionParameters::CameraModel::DJI_Air3) {
        batteryMinutes = 40.0;
    } else if (model == Models::MissionParameters::CameraModel::DJI_Mavic3) {
        batteryMinutes = 45.0;
    }

    double flightTimeMinutes = flightTimeSeconds / 60.0;
    // Use 80% of battery capacity for safety
    int batteriesNeeded = static_cast<int>(std::ceil(flightTimeMinutes / (batteryMinutes * 0.8)));

    // Update labels
    m_surveyAreaLabel->setText(QString("%1 m² (%2 ha)")
        .arg(m_surveyAreaSize, 0, 'f', 0)
        .arg(m_surveyAreaSize / 10000.0, 0, 'f', 2));

    m_estimatedDistanceLabel->setText(QString("%1 m (%2 km)")
        .arg(totalDistance, 0, 'f', 0)
        .arg(totalDistance / 1000.0, 0, 'f', 2));

    m_estimatedTimeLabel->setText(QString("%1 min (%2:%3)")
        .arg(flightTimeMinutes, 0, 'f', 1)
        .arg(static_cast<int>(flightTimeMinutes / 60))
        .arg(static_cast<int>(fmod(flightTimeMinutes, 60)), 2, 10, QChar('0')));

    m_estimatedPhotosLabel->setText(QString("%1 photos").arg(totalPhotos));

    m_estimatedBatteriesLabel->setText(QString("%1 %2")
        .arg(batteriesNeeded)
        .arg(batteriesNeeded == 1 ? "battery" : "batteries"));
}

void MissionParametersDialog::loadDefaults()
{
    m_altitudeSpinBox->setValue(100.0);
    m_speedSpinBox->setValue(10.0);
    m_maxSpeedSpinBox->setValue(15.0);
    m_takeoffAltitudeSpinBox->setValue(10.0);
    m_flightDirectionSpinBox->setValue(0.0);
    m_frontOverlapSlider->setValue(75);
    m_sideOverlapSlider->setValue(75);
    m_gimbalPitchSpinBox->setValue(-90.0);
    m_cameraAngleSpinBox->setValue(0.0);
    m_cameraModelCombo->setCurrentIndex(0);
    m_patternTypeCombo->setCurrentIndex(1);
    m_finishActionCombo->setCurrentIndex(0);
    m_reversePathCheckBox->setChecked(false);

    updateCameraInfo();
    updateEstimates();
}

void MissionParametersDialog::loadPreset(const QString& presetName)
{
    if (presetName == "mapping") {
        m_altitudeSpinBox->setValue(100.0);
        m_speedSpinBox->setValue(12.0);
        m_frontOverlapSlider->setValue(75);
        m_sideOverlapSlider->setValue(75);
        m_gimbalPitchSpinBox->setValue(-90.0);
    } else if (presetName == "inspection") {
        m_altitudeSpinBox->setValue(30.0);
        m_speedSpinBox->setValue(5.0);
        m_frontOverlapSlider->setValue(70);
        m_sideOverlapSlider->setValue(70);
        m_gimbalPitchSpinBox->setValue(-60.0);
    } else if (presetName == "highdetail") {
        m_altitudeSpinBox->setValue(60.0);
        m_speedSpinBox->setValue(8.0);
        m_frontOverlapSlider->setValue(85);
        m_sideOverlapSlider->setValue(85);
        m_gimbalPitchSpinBox->setValue(-90.0);
    }

    updateCameraInfo();
    updateEstimates();
}

Models::MissionParameters MissionParametersDialog::parameters() const
{
    Models::MissionParameters params;

    params.setFlightAltitude(m_altitudeSpinBox->value());
    params.setFlightSpeed(m_speedSpinBox->value());
    params.setFlightDirection(m_flightDirectionSpinBox->value());
    params.setReversePath(m_reversePathCheckBox->isChecked());

    params.setCameraModel(static_cast<Models::MissionParameters::CameraModel>(
        m_cameraModelCombo->currentData().toInt()));
    params.setFrontOverlap(m_frontOverlapSlider->value());
    params.setSideOverlap(m_sideOverlapSlider->value());
    params.setGimbalPitch(m_gimbalPitchSpinBox->value());
    params.setCameraAngle(m_cameraAngleSpinBox->value());

    params.setFinishAction(static_cast<Models::MissionParameters::FinishAction>(
        m_finishActionCombo->currentData().toInt()));
    params.setTakeoffAltitude(m_takeoffAltitudeSpinBox->value());
    params.setMaxSpeed(m_maxSpeedSpinBox->value());

    // Calculate path spacing based on camera and overlap
    auto model = params.cameraModel();
    const CameraSpec& spec = getCameraSpecs()[model];
    double altitude = params.flightAltitude();
    double footprintWidth = (spec.sensorWidth * altitude) / spec.focalLength;
    double spacing = footprintWidth * (1.0 - params.sideOverlap() / 100.0);
    params.setPathSpacing(spacing);

    return params;
}

void MissionParametersDialog::setParameters(const Models::MissionParameters& params)
{
    m_parameters = params;

    m_altitudeSpinBox->setValue(params.flightAltitude());
    m_speedSpinBox->setValue(params.flightSpeed());
    m_flightDirectionSpinBox->setValue(params.flightDirection());
    m_reversePathCheckBox->setChecked(params.reversePath());

    // Set camera model by finding matching index
    for (int i = 0; i < m_cameraModelCombo->count(); ++i) {
        if (static_cast<Models::MissionParameters::CameraModel>(
                m_cameraModelCombo->itemData(i).toInt()) == params.cameraModel()) {
            m_cameraModelCombo->setCurrentIndex(i);
            break;
        }
    }

    m_frontOverlapSlider->setValue(static_cast<int>(params.frontOverlap()));
    m_sideOverlapSlider->setValue(static_cast<int>(params.sideOverlap()));
    m_gimbalPitchSpinBox->setValue(params.gimbalPitch());
    m_cameraAngleSpinBox->setValue(params.cameraAngle());

    // Set finish action
    for (int i = 0; i < m_finishActionCombo->count(); ++i) {
        if (static_cast<Models::MissionParameters::FinishAction>(
                m_finishActionCombo->itemData(i).toInt()) == params.finishAction()) {
            m_finishActionCombo->setCurrentIndex(i);
            break;
        }
    }

    m_takeoffAltitudeSpinBox->setValue(params.takeoffAltitude());
    m_maxSpeedSpinBox->setValue(params.maxSpeed());

    updateCameraInfo();
    updateEstimates();
}

void MissionParametersDialog::setPatternType(Models::FlightPlan::PatternType type)
{
    m_patternType = type;

    for (int i = 0; i < m_patternTypeCombo->count(); ++i) {
        if (static_cast<Models::FlightPlan::PatternType>(
                m_patternTypeCombo->itemData(i).toInt()) == type) {
            m_patternTypeCombo->setCurrentIndex(i);
            break;
        }
    }
}

void MissionParametersDialog::setSurveyAreaSize(double squareMeters)
{
    m_surveyAreaSize = squareMeters;
    updateEstimates();
}

double MissionParametersDialog::estimatedFlightTime() const
{
    // Parse from label (in minutes)
    QString text = m_estimatedTimeLabel->text();
    if (text.contains("Not calculated")) {
        return 0.0;
    }
    // Extract number before "min"
    int minIndex = text.indexOf(" min");
    if (minIndex > 0) {
        return text.left(minIndex).toDouble();
    }
    return 0.0;
}

double MissionParametersDialog::estimatedDistance() const
{
    // Parse from label (in meters)
    QString text = m_estimatedDistanceLabel->text();
    if (text.contains("Not calculated")) {
        return 0.0;
    }
    // Extract number before " m"
    int mIndex = text.indexOf(" m");
    if (mIndex > 0) {
        return text.left(mIndex).toDouble();
    }
    return 0.0;
}

int MissionParametersDialog::estimatedPhotoCount() const
{
    // Parse from label
    QString text = m_estimatedPhotosLabel->text();
    if (text.contains("Not calculated")) {
        return 0;
    }
    // Extract number before " photos"
    int photosIndex = text.indexOf(" photos");
    if (photosIndex > 0) {
        return text.left(photosIndex).toInt();
    }
    return 0;
}

void MissionParametersDialog::onPresetSelected()
{
    // Handled by lambda connections
}

} // namespace UI
} // namespace DroneMapper
