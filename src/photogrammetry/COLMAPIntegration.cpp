#include "COLMAPIntegration.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QElapsedTimer>

namespace DroneMapper {
namespace Photogrammetry {

COLMAPConfig::COLMAPConfig()
    : useGPU(true)
    , gpuIndex(0)
    , cameraModel("OPENCV")
    , maxImageSize(3200)
    , maxNumFeatures(8192)
    , exhaustiveMatching(false)
    , matchingWindowSize(10)
    , maxImageSizeDense(2000)
    , numThreads(-1)  // Auto-detect
    , geometricConsistency(true)
    , poissonDepth(13)
    , coloredMesh(true)
{
    colmapExecutable = COLMAPIntegration::findCOLMAPExecutable();
}

QString COLMAPConfig::validate() const
{
    if (colmapExecutable.isEmpty()) {
        return "COLMAP executable not found";
    }

    if (!QFileInfo::exists(colmapExecutable)) {
        return QString("COLMAP executable does not exist: %1").arg(colmapExecutable);
    }

    if (workspacePath.isEmpty()) {
        return "Workspace path not set";
    }

    if (imagePath.isEmpty()) {
        return "Image path not set";
    }

    if (!QDir(imagePath).exists()) {
        return QString("Image directory does not exist: %1").arg(imagePath);
    }

    QDir imageDir(imagePath);
    QStringList images = imageDir.entryList(QStringList() << "*.jpg" << "*.JPG" << "*.png" << "*.PNG", QDir::Files);
    if (images.isEmpty()) {
        return "No images found in image directory";
    }

    return QString(); // Valid
}

QString COLMAPStatus::getStatusString() const
{
    if (hasFailed) return "Failed";
    if (isComplete) return "Complete";
    if (isRunning) return "Running";
    return "Not Started";
}

QString COLMAPResults::getSummary() const
{
    QString summary;
    summary += QString("=== COLMAP RECONSTRUCTION RESULTS ===\n\n");

    if (!success) {
        summary += "Status: FAILED\n";
        summary += QString("Error: %1\n").arg(errorLog);
        return summary;
    }

    summary += "Status: SUCCESS\n\n";

    summary += "SPARSE RECONSTRUCTION:\n";
    summary += QString("  Cameras: %1\n").arg(numCameras);
    summary += QString("  Images: %1\n").arg(numImages);
    summary += QString("  3D Points: %1\n").arg(numPoints3D);
    summary += QString("  Mean Reprojection Error: %1 px\n\n").arg(meanReprojectionError, 0, 'f', 3);

    if (numDensePoints > 0) {
        summary += "DENSE RECONSTRUCTION:\n";
        summary += QString("  Dense Points: %1\n").arg(numDensePoints);
        summary += QString("  Point Cloud: %1\n\n").arg(fusedPointCloudPath);
    }

    if (numVertices > 0) {
        summary += "MESH:\n";
        summary += QString("  Vertices: %1\n").arg(numVertices);
        summary += QString("  Faces: %1\n").arg(numFaces);
        summary += QString("  Mesh File: %1\n").arg(meshPath);
    }

    return summary;
}

QString COLMAPIntegration::findCOLMAPExecutable()
{
    // Try common locations
    QStringList possiblePaths;

    possiblePaths << "colmap";  // In PATH
    possiblePaths << "/usr/bin/colmap";
    possiblePaths << "/usr/local/bin/colmap";
    possiblePaths << QDir::homePath() + "/colmap/build/src/exe/colmap";

#ifdef Q_OS_WIN
    possiblePaths << "C:/Program Files/COLMAP/COLMAP.bat";
    possiblePaths << "C:/COLMAP/COLMAP.bat";
#endif

    for (const QString& path : possiblePaths) {
        QProcess process;
        process.start(path, QStringList() << "--version");
        process.waitForFinished(2000);

        if (process.exitCode() == 0 || process.exitCode() == 1) {
            // COLMAP found (exit code 1 is ok for --version)
            return path;
        }
    }

    return QString(); // Not found
}

COLMAPIntegration::COLMAPIntegration(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
{
    m_status.isRunning = false;
    m_status.isComplete = false;
    m_status.hasFailed = false;
    m_status.progress = 0.0;
}

COLMAPIntegration::~COLMAPIntegration()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished();
        delete m_process;
    }
}

bool COLMAPIntegration::isCOLMAPInstalled()
{
    return !findCOLMAPExecutable().isEmpty();
}

QString COLMAPIntegration::getCOLMAPVersion()
{
    QString executable = findCOLMAPExecutable();
    if (executable.isEmpty()) {
        return "Not installed";
    }

    QProcess process;
    process.start(executable, QStringList() << "--version");
    process.waitForFinished(2000);

    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();

    // Parse version from output
    QRegularExpression versionRegex(R"(COLMAP\s+(\d+\.\d+))");
    QRegularExpressionMatch match = versionRegex.match(output + errorOutput);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    return "Unknown";
}

COLMAPResults COLMAPIntegration::runFullPipeline(const COLMAPConfig& config)
{
    m_config = config;
    m_results = COLMAPResults();

    QString validationError = config.validate();
    if (!validationError.isEmpty()) {
        m_results.success = false;
        m_results.errorLog = validationError;
        return m_results;
    }

    // Create workspace directories
    QDir().mkpath(config.workspacePath);
    QDir().mkpath(config.sparsePath);
    QDir().mkpath(config.densePath);

    // Run pipeline stages
    QList<COLMAPStage> stages = {
        COLMAPStage::FeatureExtraction,
        COLMAPStage::FeatureMatching,
        COLMAPStage::SparseReconstruction,
        COLMAPStage::ImageUndistortion,
        COLMAPStage::DenseReconstruction
    };

    for (COLMAPStage stage : stages) {
        if (!runStage(stage, config)) {
            m_results.success = false;
            return m_results;
        }

        if (m_status.hasFailed) {
            m_results.success = false;
            return m_results;
        }
    }

    m_results.success = true;
    m_status.isComplete = true;

    emit pipelineCompleted(m_results);

    return m_results;
}

bool COLMAPIntegration::runStage(COLMAPStage stage, const COLMAPConfig& config)
{
    m_config = config;
    m_status.currentStage = stage;
    m_status.stageDescription = getStageDescription(stage);
    m_status.isRunning = true;

    emit stageStarted(stage);

    QString command;

    switch (stage) {
    case COLMAPStage::FeatureExtraction:
        command = buildFeatureExtractionCommand(config);
        break;
    case COLMAPStage::FeatureMatching:
        command = buildFeatureMatchingCommand(config);
        break;
    case COLMAPStage::SparseReconstruction:
        command = buildMapperCommand(config);
        break;
    case COLMAPStage::ImageUndistortion:
        command = buildImageUndistortionCommand(config);
        break;
    case COLMAPStage::DenseReconstruction:
        command = buildDenseReconstructionCommand(config);
        break;
    case COLMAPStage::MeshReconstruction:
        command = buildMeshReconstructionCommand(config);
        break;
    default:
        return false;
    }

    bool success = executeCommand(command, stage);

    if (success) {
        emit stageCompleted(stage);
    }

    m_status.isRunning = false;

    return success;
}

void COLMAPIntegration::cancel()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->kill();
        m_status.isRunning = false;
        m_status.hasFailed = true;
        m_status.errorMessage = "Cancelled by user";
    }
}

void COLMAPIntegration::onProcessReadyReadStandardOutput()
{
    if (!m_process) return;

    QString output = m_process->readAllStandardOutput();
    m_currentStdout += output;

    parseProgressFromOutput(output, m_status.currentStage);
}

void COLMAPIntegration::onProcessReadyReadStandardError()
{
    if (!m_process) return;

    QString output = m_process->readAllStandardError();
    m_currentStderr += output;

    // COLMAP sometimes writes progress to stderr
    parseProgressFromOutput(output, m_status.currentStage);
}

void COLMAPIntegration::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        m_status.hasFailed = true;
        m_status.errorMessage = QString("Process failed with exit code %1: %2")
            .arg(exitCode)
            .arg(m_currentStderr);

        emit errorOccurred(m_status.errorMessage);
    }
}

QString COLMAPIntegration::buildFeatureExtractionCommand(const COLMAPConfig& config)
{
    QStringList args;
    args << config.colmapExecutable;
    args << "feature_extractor";
    args << "--database_path" << config.databasePath;
    args << "--image_path" << config.imagePath;
    args << "--ImageReader.camera_model" << config.cameraModel;
    args << "--ImageReader.single_camera" << "0";  // Multiple cameras
    args << "--SiftExtraction.max_image_size" << QString::number(config.maxImageSize);
    args << "--SiftExtraction.max_num_features" << QString::number(config.maxNumFeatures);

    if (config.useGPU) {
        args << "--SiftExtraction.use_gpu" << "1";
        args << "--SiftExtraction.gpu_index" << QString::number(config.gpuIndex);
    } else {
        args << "--SiftExtraction.use_gpu" << "0";
    }

    return args.join(" ");
}

QString COLMAPIntegration::buildFeatureMatchingCommand(const COLMAPConfig& config)
{
    QStringList args;
    args << config.colmapExecutable;

    if (config.exhaustiveMatching) {
        args << "exhaustive_matcher";
    } else {
        args << "sequential_matcher";
        args << "--SequentialMatching.overlap" << QString::number(config.matchingWindowSize);
    }

    args << "--database_path" << config.databasePath;

    if (config.useGPU) {
        args << "--SiftMatching.use_gpu" << "1";
        args << "--SiftMatching.gpu_index" << QString::number(config.gpuIndex);
    } else {
        args << "--SiftMatching.use_gpu" << "0";
    }

    return args.join(" ");
}

QString COLMAPIntegration::buildMapperCommand(const COLMAPConfig& config)
{
    QStringList args;
    args << config.colmapExecutable;
    args << "mapper";
    args << "--database_path" << config.databasePath;
    args << "--image_path" << config.imagePath;
    args << "--output_path" << config.sparsePath;

    return args.join(" ");
}

QString COLMAPIntegration::buildImageUndistortionCommand(const COLMAPConfig& config)
{
    QStringList args;
    args << config.colmapExecutable;
    args << "image_undistorter";
    args << "--image_path" << config.imagePath;
    args << "--input_path" << config.sparsePath + "/0";  // First model
    args << "--output_path" << config.densePath;
    args << "--output_type" << "COLMAP";

    return args.join(" ");
}

QString COLMAPIntegration::buildDenseReconstructionCommand(const COLMAPConfig& config)
{
    QStringList args;
    args << config.colmapExecutable;
    args << "patch_match_stereo";
    args << "--workspace_path" << config.densePath;
    args << "--workspace_format" << "COLMAP";
    args << "--PatchMatchStereo.max_image_size" << QString::number(config.maxImageSizeDense);

    if (config.geometricConsistency) {
        args << "--PatchMatchStereo.geom_consistency" << "true";
    }

    if (config.useGPU) {
        args << "--PatchMatchStereo.gpu_index" << QString::number(config.gpuIndex);
    }

    return args.join(" ");
}

QString COLMAPIntegration::buildMeshReconstructionCommand(const COLMAPConfig& config)
{
    QStringList args;
    args << config.colmapExecutable;
    args << "poisson_mesher";
    args << "--input_path" << config.densePath + "/fused.ply";
    args << "--output_path" << config.densePath + "/meshed-poisson.ply";
    args << "--PoissonMeshing.depth" << QString::number(config.poissonDepth);

    return args.join(" ");
}

bool COLMAPIntegration::executeCommand(const QString& command, COLMAPStage stage)
{
    Q_UNUSED(stage);

    if (m_process) {
        delete m_process;
    }

    m_process = new QProcess(this);
    m_currentStdout.clear();
    m_currentStderr.clear();

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &COLMAPIntegration::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &COLMAPIntegration::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &COLMAPIntegration::onProcessFinished);

    m_process->start("/bin/sh", QStringList() << "-c" << command);

    if (!m_process->waitForStarted()) {
        m_status.hasFailed = true;
        m_status.errorMessage = "Failed to start COLMAP process";
        return false;
    }

    // Wait for process to finish (with timeout for very long operations)
    m_process->waitForFinished(-1);  // No timeout

    return (m_process->exitCode() == 0);
}

void COLMAPIntegration::parseProgressFromOutput(const QString& output, COLMAPStage stage)
{
    Q_UNUSED(stage);

    // Parse progress from COLMAP output
    // COLMAP typically outputs progress like "Processing image [123/456]"

    QRegularExpression progressRegex(R"(\[(\d+)/(\d+)\])");
    QRegularExpressionMatch match = progressRegex.match(output);

    if (match.hasMatch()) {
        int current = match.captured(1).toInt();
        int total = match.captured(2).toInt();

        if (total > 0) {
            double progress = (current * 100.0) / total;
            updateProgress(progress, QString("Processing %1/%2").arg(current).arg(total));
        }
    }
}

void COLMAPIntegration::updateProgress(double progress, const QString& message)
{
    m_status.progress = progress;
    m_status.currentStep = message;

    emit progressUpdated(progress, message);
}

bool COLMAPIntegration::validateResults(COLMAPStage stage)
{
    Q_UNUSED(stage);

    // Simplified validation - would check output files exist
    return true;
}

QString COLMAPIntegration::getStageDescription(COLMAPStage stage) const
{
    switch (stage) {
    case COLMAPStage::FeatureExtraction:
        return "Extracting SIFT features from images";
    case COLMAPStage::FeatureMatching:
        return "Matching features between images";
    case COLMAPStage::SparseReconstruction:
        return "Running Structure from Motion (SfM)";
    case COLMAPStage::ImageUndistortion:
        return "Undistorting images for dense reconstruction";
    case COLMAPStage::DenseReconstruction:
        return "Running Multi-View Stereo (MVS)";
    case COLMAPStage::MeshReconstruction:
        return "Reconstructing mesh surface";
    case COLMAPStage::TextureMapping:
        return "Mapping textures onto mesh";
    }
    return "Unknown stage";
}

} // namespace Photogrammetry
} // namespace DroneMapper
