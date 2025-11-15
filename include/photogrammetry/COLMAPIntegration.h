#ifndef COLMAPINTEGRATION_H
#define COLMAPINTEGRATION_H

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QObject>

namespace DroneMapper {
namespace Photogrammetry {

/**
 * @brief COLMAP processing stages
 */
enum class COLMAPStage {
    FeatureExtraction,      // Extract SIFT features
    FeatureMatching,        // Match features between images
    SparseReconstruction,   // Structure from Motion (SfM)
    ImageUndistortion,      // Undistort images
    DenseReconstruction,    // Multi-View Stereo (MVS)
    MeshReconstruction,     // Poisson surface reconstruction
    TextureMapping          // Texture the mesh
};

/**
 * @brief COLMAP configuration
 */
struct COLMAPConfig {
    QString colmapExecutable;       // Path to COLMAP binary
    QString workspacePath;          // Working directory
    QString imagePath;              // Input images directory
    QString databasePath;           // Database file path
    QString sparsePath;             // Sparse reconstruction output
    QString densePath;              // Dense reconstruction output

    bool useGPU;
    int gpuIndex;
    QString cameraModel;            // "OPENCV", "PINHOLE", "RADIAL", etc.

    // Feature extraction params
    int maxImageSize;               // Max dimension (default: 3200)
    int maxNumFeatures;             // Max features per image (default: 8192)

    // Matching params
    bool exhaustiveMatching;        // vs sequential/spatial
    int matchingWindowSize;         // For sequential matching

    // Dense reconstruction params
    int maxImageSizeDense;          // Max size for MVS (default: 2000)
    int numThreads;                 // CPU threads
    bool geometricConsistency;      // Enforce geometric consistency

    // Mesh params
    int poissonDepth;               // Poisson octree depth (default: 13)
    bool coloredMesh;               // Vertex coloring

    COLMAPConfig();
    QString validate() const;       // Returns error message or empty if valid
};

/**
 * @brief COLMAP process status
 */
struct COLMAPStatus {
    COLMAPStage currentStage;
    QString stageDescription;
    double progress;                // 0-100%
    QString currentStep;
    int processedImages;
    int totalImages;
    double elapsedSeconds;
    QString estimatedTimeRemaining;

    bool isRunning;
    bool isComplete;
    bool hasFailed;
    QString errorMessage;

    QString getStatusString() const;
};

/**
 * @brief COLMAP reconstruction results
 */
struct COLMAPResults {
    bool success;

    // Sparse reconstruction
    int numCameras;
    int numImages;
    int numPoints3D;
    double meanReprojectionError;

    // Dense reconstruction
    QString depthMapsPath;
    QString fusedPointCloudPath;     // .ply file
    int numDensePoints;

    // Mesh
    QString meshPath;                // .ply file
    int numVertices;
    int numFaces;

    QString logPath;
    QString errorLog;

    QString getSummary() const;
};

/**
 * @brief Integrates COLMAP photogrammetry pipeline
 *
 * Features:
 * - Automatic COLMAP command generation
 * - Process monitoring and progress tracking
 * - GPU acceleration support
 * - Error detection and recovery
 * - Log file parsing
 * - Incremental processing (resume support)
 * - Quality assessment
 * - Output validation
 *
 * Pipeline stages:
 * 1. Feature Extraction (SIFT/others)
 * 2. Feature Matching (exhaustive/sequential/spatial)
 * 3. Sparse Reconstruction (SfM)
 * 4. Image Undistortion
 * 5. Dense Reconstruction (MVS)
 * 6. Mesh Reconstruction (Poisson)
 * 7. Texture Mapping
 *
 * Requires: COLMAP installed (https://colmap.github.io/)
 */
class COLMAPIntegration : public QObject {
    Q_OBJECT

public:
    explicit COLMAPIntegration(QObject* parent = nullptr);
    ~COLMAPIntegration() override;

    /**
     * @brief Check if COLMAP is installed
     * @return True if COLMAP executable found
     */
    static bool isCOLMAPInstalled();

    /**
     * @brief Get COLMAP version
     * @return Version string (e.g., "3.8")
     */
    static QString getCOLMAPVersion();

    /**
     * @brief Find COLMAP executable
     * @return Path to COLMAP binary
     */
    static QString findCOLMAPExecutable();

    /**
     * @brief Run full COLMAP pipeline
     * @param config Configuration
     * @return Results
     */
    COLMAPResults runFullPipeline(const COLMAPConfig& config);

    /**
     * @brief Run specific COLMAP stage
     * @param stage Stage to run
     * @param config Configuration
     * @return True on success
     */
    bool runStage(COLMAPStage stage, const COLMAPConfig& config);

    /**
     * @brief Get current status
     * @return Processing status
     */
    COLMAPStatus getStatus() const { return m_status; }

    /**
     * @brief Cancel processing
     */
    void cancel();

    /**
     * @brief Check if processing is running
     * @return True if running
     */
    bool isRunning() const { return m_status.isRunning; }

signals:
    void stageStarted(COLMAPStage stage);
    void progressUpdated(double progress, const QString& message);
    void stageCompleted(COLMAPStage stage);
    void pipelineCompleted(const COLMAPResults& results);
    void errorOccurred(const QString& error);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess* m_process;
    COLMAPConfig m_config;
    COLMAPStatus m_status;
    COLMAPResults m_results;
    QString m_currentStdout;
    QString m_currentStderr;

    // Command builders
    QString buildFeatureExtractionCommand(const COLMAPConfig& config);
    QString buildFeatureMatchingCommand(const COLMAPConfig& config);
    QString buildMapperCommand(const COLMAPConfig& config);
    QString buildImageUndistortionCommand(const COLMAPConfig& config);
    QString buildDenseReconstructionCommand(const COLMAPConfig& config);
    QString buildMeshReconstructionCommand(const COLMAPConfig& config);

    // Helpers
    bool executeCommand(const QString& command, COLMAPStage stage);
    void parseProgressFromOutput(const QString& output, COLMAPStage stage);
    void updateProgress(double progress, const QString& message);
    bool validateResults(COLMAPStage stage);

    QString getStageDescription(COLMAPStage stage) const;
};

} // namespace Photogrammetry
} // namespace DroneMapper

#endif // COLMAPINTEGRATION_H
