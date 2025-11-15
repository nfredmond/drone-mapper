#ifndef GPUDETECTOR_H
#define GPUDETECTOR_H

#include <QString>
#include <QList>

namespace DroneMapper {
namespace Photogrammetry {

/**
 * @brief GPU types
 */
enum class GPUType {
    NVIDIA_CUDA,    // NVIDIA with CUDA support
    AMD_OpenCL,     // AMD with OpenCL
    Intel,          // Intel integrated
    Apple_Metal,    // Apple Silicon
    Unknown
};

/**
 * @brief GPU information
 */
struct GPUInfo {
    QString name;
    GPUType type;
    int deviceId;
    size_t totalMemoryMB;
    size_t freeMemoryMB;
    int computeCapability;      // For CUDA (e.g., 75 = 7.5)
    int cudaCores;              // Estimated CUDA cores
    int clockSpeedMHz;
    QString driverVersion;
    bool supportsCUDA;
    bool supportsOpenCL;
    bool supportsVulkan;

    QString getTypeString() const;
    QString getSummary() const;
    bool isHighPerformance() const;
};

/**
 * @brief System GPU capabilities
 */
struct GPUCapabilities {
    QList<GPUInfo> gpus;
    bool hasCUDA;
    bool hasOpenCL;
    bool hasVulkan;
    int recommendedGPU;         // Index in gpus list (-1 if none)
    size_t totalGPUMemoryMB;

    QString getSummary() const;
    GPUInfo getBestGPU() const;
};

/**
 * @brief Processing recommendations based on GPU
 */
struct ProcessingRecommendations {
    bool useGPU;
    int gpuDeviceId;
    QString backend;            // "CUDA", "OpenCL", "CPU"
    int recommendedBatchSize;
    int recommendedThreads;
    bool enableHalfPrecision;   // Use FP16 instead of FP32
    QString reason;

    QString getReport() const;
};

/**
 * @brief Detects and manages GPU capabilities
 *
 * Features:
 * - CUDA detection and capabilities
 * - OpenCL detection
 * - GPU memory querying
 * - Multi-GPU support
 * - Compute capability checking
 * - Performance profiling
 * - Automatic backend selection
 * - Processing recommendations
 *
 * Supported backends:
 * - NVIDIA CUDA (via nvidia-smi/CUDA runtime)
 * - AMD OpenCL
 * - CPU fallback
 * - Apple Metal (macOS)
 */
class GPUDetector {
public:
    GPUDetector();

    /**
     * @brief Detect all available GPUs
     * @return GPU capabilities
     */
    static GPUCapabilities detectGPUs();

    /**
     * @brief Detect CUDA capabilities
     * @return List of CUDA-capable GPUs
     */
    static QList<GPUInfo> detectCUDADevices();

    /**
     * @brief Detect OpenCL devices
     * @return List of OpenCL-capable GPUs
     */
    static QList<GPUInfo> detectOpenCLDevices();

    /**
     * @brief Get CUDA version
     * @return CUDA version string (e.g., "11.8")
     */
    static QString getCUDAVersion();

    /**
     * @brief Check if CUDA is available
     * @return True if CUDA runtime detected
     */
    static bool isCUDAAvailable();

    /**
     * @brief Check if OpenCL is available
     * @return True if OpenCL runtime detected
     */
    static bool isOpenCLAvailable();

    /**
     * @brief Get GPU memory usage
     * @param deviceId GPU device ID
     * @return Free memory in MB
     */
    static size_t getGPUFreeMemory(int deviceId = 0);

    /**
     * @brief Get processing recommendations
     * @param capabilities GPU capabilities
     * @param estimatedMemoryNeedMB Estimated memory requirement
     * @return Recommendations for processing
     */
    static ProcessingRecommendations getProcessingRecommendations(
        const GPUCapabilities& capabilities,
        size_t estimatedMemoryNeedMB = 4096);

    /**
     * @brief Benchmark GPU performance
     * @param deviceId GPU to benchmark
     * @return Performance score (0-100)
     */
    static double benchmarkGPU(int deviceId = 0);

    /**
     * @brief Get recommended batch size for GPU
     * @param deviceId GPU device ID
     * @param itemSizeMB Size of each item in MB
     * @return Recommended batch size
     */
    static int getRecommendedBatchSize(
        int deviceId,
        size_t itemSizeMB);

    /**
     * @brief Check if GPU has sufficient memory
     * @param deviceId GPU device ID
     * @param requiredMemoryMB Required memory in MB
     * @return True if sufficient memory available
     */
    static bool hasSufficientMemory(
        int deviceId,
        size_t requiredMemoryMB);

    /**
     * @brief Get optimal number of processing threads
     * @param useGPU Whether GPU will be used
     * @return Recommended thread count
     */
    static int getOptimalThreadCount(bool useGPU);

    /**
     * @brief Generate system report
     * @param capabilities GPU capabilities
     * @return Detailed system report
     */
    static QString generateSystemReport(
        const GPUCapabilities& capabilities);

private:
    static QList<GPUInfo> detectNVIDIAGPUs();
    static QList<GPUInfo> detectAMDGPUs();
    static QList<GPUInfo> detectIntelGPUs();

    static GPUInfo parseNvidiaSMIOutput(const QString& output);
    static GPUInfo parseLspciOutput(const QString& output);

    static QString executeCommand(const QString& command);

    static int selectBestGPU(const QList<GPUInfo>& gpus);

    static double calculatePerformanceScore(const GPUInfo& gpu);
};

} // namespace Photogrammetry
} // namespace DroneMapper

#endif // GPUDETECTOR_H
