#include "GPUDetector.h"
#include <QProcess>
#include <QThread>
#include <cmath>

namespace DroneMapper {
namespace Photogrammetry {

QString GPUInfo::getTypeString() const
{
    switch (type) {
    case GPUType::NVIDIA_CUDA:  return "NVIDIA CUDA";
    case GPUType::AMD_OpenCL:   return "AMD OpenCL";
    case GPUType::Intel:        return "Intel";
    case GPUType::Apple_Metal:  return "Apple Metal";
    case GPUType::Unknown:      return "Unknown";
    }
    return "Unknown";
}

QString GPUInfo::getSummary() const
{
    return QString("%1 - %2 MB VRAM - %3")
        .arg(name)
        .arg(totalMemoryMB)
        .arg(getTypeString());
}

bool GPUInfo::isHighPerformance() const
{
    // High performance if:
    // - Has dedicated VRAM > 4GB
    // - CUDA cores > 1000 (or equivalent)
    // - Supports CUDA or has high compute capability

    if (totalMemoryMB < 4096) return false;

    if (supportsCUDA && cudaCores > 1000) return true;
    if (type == GPUType::NVIDIA_CUDA && computeCapability >= 60) return true;

    return false;
}

QString GPUCapabilities::getSummary() const
{
    QString summary;
    summary += QString("GPUs detected: %1\n").arg(gpus.count());
    summary += QString("CUDA available: %1\n").arg(hasCUDA ? "Yes" : "No");
    summary += QString("OpenCL available: %1\n").arg(hasOpenCL ? "Yes" : "No");
    summary += QString("Total GPU memory: %1 MB\n").arg(totalGPUMemoryMB);

    if (recommendedGPU >= 0 && recommendedGPU < gpus.count()) {
        summary += QString("Recommended GPU: %1\n").arg(gpus[recommendedGPU].name);
    }

    return summary;
}

GPUInfo GPUCapabilities::getBestGPU() const
{
    if (recommendedGPU >= 0 && recommendedGPU < gpus.count()) {
        return gpus[recommendedGPU];
    }

    // Return first GPU if no recommendation
    if (!gpus.isEmpty()) {
        return gpus.first();
    }

    return GPUInfo();
}

QString ProcessingRecommendations::getReport() const
{
    QString report;
    report += "=== PROCESSING RECOMMENDATIONS ===\n\n";
    report += QString("Use GPU: %1\n").arg(useGPU ? "Yes" : "No");

    if (useGPU) {
        report += QString("GPU Device ID: %1\n").arg(gpuDeviceId);
        report += QString("Backend: %1\n").arg(backend);
        report += QString("Batch Size: %1\n").arg(recommendedBatchSize);
        report += QString("Half Precision: %1\n").arg(enableHalfPrecision ? "Yes" : "No");
    } else {
        report += QString("CPU Threads: %1\n").arg(recommendedThreads);
    }

    report += QString("\nReason: %1\n").arg(reason);

    return report;
}

GPUDetector::GPUDetector()
{
}

GPUCapabilities GPUDetector::detectGPUs()
{
    GPUCapabilities caps;
    caps.hasCUDA = false;
    caps.hasOpenCL = false;
    caps.hasVulkan = false;
    caps.recommendedGPU = -1;
    caps.totalGPUMemoryMB = 0;

    // Try to detect NVIDIA GPUs first (most common for photogrammetry)
    QList<GPUInfo> nvidiaGPUs = detectNVIDIAGPUs();
    caps.gpus.append(nvidiaGPUs);
    if (!nvidiaGPUs.isEmpty()) {
        caps.hasCUDA = true;
    }

    // Detect AMD GPUs
    QList<GPUInfo> amdGPUs = detectAMDGPUs();
    caps.gpus.append(amdGPUs);
    if (!amdGPUs.isEmpty()) {
        caps.hasOpenCL = true;
    }

    // Detect Intel GPUs
    QList<GPUInfo> intelGPUs = detectIntelGPUs();
    caps.gpus.append(intelGPUs);

    // Calculate total memory
    for (const auto& gpu : caps.gpus) {
        caps.totalGPUMemoryMB += gpu.totalMemoryMB;
    }

    // Select best GPU
    caps.recommendedGPU = selectBestGPU(caps.gpus);

    return caps;
}

QList<GPUInfo> GPUDetector::detectCUDADevices()
{
    return detectNVIDIAGPUs();
}

QList<GPUInfo> GPUDetector::detectOpenCLDevices()
{
    QList<GPUInfo> devices;

    // Simplified - would use OpenCL API in production
    // For now, return AMD and Intel GPUs
    devices.append(detectAMDGPUs());
    devices.append(detectIntelGPUs());

    return devices;
}

QString GPUDetector::getCUDAVersion()
{
    QString output = executeCommand("nvcc --version");

    if (output.contains("release")) {
        // Parse CUDA version from nvcc output
        // Example: "Cuda compilation tools, release 11.8, V11.8.89"
        int releaseIdx = output.indexOf("release");
        if (releaseIdx >= 0) {
            QString versionPart = output.mid(releaseIdx + 8, 10);
            int commaIdx = versionPart.indexOf(',');
            if (commaIdx > 0) {
                return versionPart.left(commaIdx).trimmed();
            }
        }
    }

    return "Unknown";
}

bool GPUDetector::isCUDAAvailable()
{
    // Check if nvidia-smi exists
    QString output = executeCommand("nvidia-smi --version");
    return !output.isEmpty() && output.contains("NVIDIA");
}

bool GPUDetector::isOpenCLAvailable()
{
    // Simplified check - would use OpenCL API in production
    return !detectAMDGPUs().isEmpty() || !detectIntelGPUs().isEmpty();
}

size_t GPUDetector::getGPUFreeMemory(int deviceId)
{
    Q_UNUSED(deviceId);

    // Try nvidia-smi
    QString output = executeCommand("nvidia-smi --query-gpu=memory.free --format=csv,noheader,nounits");

    if (!output.isEmpty()) {
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        if (deviceId < lines.count()) {
            return lines[deviceId].trimmed().toULongLong();
        }
    }

    return 0;
}

ProcessingRecommendations GPUDetector::getProcessingRecommendations(
    const GPUCapabilities& capabilities,
    size_t estimatedMemoryNeedMB)
{
    ProcessingRecommendations recs;
    recs.useGPU = false;
    recs.gpuDeviceId = 0;
    recs.backend = "CPU";
    recs.recommendedBatchSize = 1;
    recs.recommendedThreads = getOptimalThreadCount(false);
    recs.enableHalfPrecision = false;

    if (capabilities.gpus.isEmpty()) {
        recs.reason = "No GPU detected - using CPU";
        return recs;
    }

    GPUInfo bestGPU = capabilities.getBestGPU();

    // Check if GPU has sufficient memory
    if (bestGPU.totalMemoryMB < estimatedMemoryNeedMB) {
        recs.reason = QString("GPU has insufficient memory (%1 MB available, %2 MB needed) - using CPU")
            .arg(bestGPU.totalMemoryMB)
            .arg(estimatedMemoryNeedMB);
        return recs;
    }

    // Recommend GPU processing
    recs.useGPU = true;
    recs.gpuDeviceId = capabilities.recommendedGPU;

    if (capabilities.hasCUDA) {
        recs.backend = "CUDA";
        recs.enableHalfPrecision = (bestGPU.computeCapability >= 70); // Tensor cores
        recs.reason = "CUDA-capable GPU detected - excellent performance expected";
    } else if (capabilities.hasOpenCL) {
        recs.backend = "OpenCL";
        recs.reason = "OpenCL-capable GPU detected - good performance expected";
    } else {
        recs.useGPU = false;
        recs.backend = "CPU";
        recs.reason = "GPU detected but no supported compute backend - using CPU";
        return recs;
    }

    // Calculate batch size
    recs.recommendedBatchSize = getRecommendedBatchSize(
        recs.gpuDeviceId,
        estimatedMemoryNeedMB / 10); // Assume 10 items per batch initially

    recs.recommendedThreads = 1; // GPU handles parallelism

    return recs;
}

double GPUDetector::benchmarkGPU(int deviceId)
{
    Q_UNUSED(deviceId);

    // Simplified benchmark - would run actual compute tests in production
    // Return score based on detected capabilities

    return 50.0; // Default moderate score
}

int GPUDetector::getRecommendedBatchSize(int deviceId, size_t itemSizeMB)
{
    size_t freeMemory = getGPUFreeMemory(deviceId);

    if (freeMemory == 0 || itemSizeMB == 0) {
        return 1; // Conservative fallback
    }

    // Use 70% of free memory for batch processing
    size_t usableMemory = freeMemory * 0.7;
    int batchSize = static_cast<int>(usableMemory / itemSizeMB);

    // Clamp to reasonable range
    return std::max(1, std::min(batchSize, 64));
}

bool GPUDetector::hasSufficientMemory(int deviceId, size_t requiredMemoryMB)
{
    size_t freeMemory = getGPUFreeMemory(deviceId);
    return freeMemory >= requiredMemoryMB;
}

int GPUDetector::getOptimalThreadCount(bool useGPU)
{
    if (useGPU) {
        return 1; // GPU handles parallelism internally
    }

    // CPU: use physical cores, not hyperthreads
    int cores = QThread::idealThreadCount();
    return std::max(1, cores);
}

QString GPUDetector::generateSystemReport(const GPUCapabilities& capabilities)
{
    QString report;
    report += "=== GPU SYSTEM REPORT ===\n\n";

    report += capabilities.getSummary();
    report += "\n";

    if (!capabilities.gpus.isEmpty()) {
        report += "DETECTED GPUS:\n";
        for (int i = 0; i < capabilities.gpus.count(); ++i) {
            const auto& gpu = capabilities.gpus[i];
            report += QString("\nGPU %1:\n").arg(i);
            report += QString("  Name: %1\n").arg(gpu.name);
            report += QString("  Type: %1\n").arg(gpu.getTypeString());
            report += QString("  Memory: %1 MB\n").arg(gpu.totalMemoryMB);
            report += QString("  CUDA Support: %1\n").arg(gpu.supportsCUDA ? "Yes" : "No");
            if (gpu.supportsCUDA) {
                report += QString("  Compute Capability: %1.%2\n")
                    .arg(gpu.computeCapability / 10)
                    .arg(gpu.computeCapability % 10);
                report += QString("  CUDA Cores: ~%1\n").arg(gpu.cudaCores);
            }
        }
    } else {
        report += "No GPUs detected\n";
    }

    return report;
}

QList<GPUInfo> GPUDetector::detectNVIDIAGPUs()
{
    QList<GPUInfo> gpus;

    QString output = executeCommand("nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader");

    if (output.isEmpty()) {
        return gpus; // No NVIDIA GPUs or nvidia-smi not available
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (int i = 0; i < lines.count(); ++i) {
        QStringList parts = lines[i].split(',');

        if (parts.count() >= 2) {
            GPUInfo gpu;
            gpu.name = parts[0].trimmed();
            gpu.type = GPUType::NVIDIA_CUDA;
            gpu.deviceId = i;
            gpu.supportsCUDA = true;
            gpu.supportsOpenCL = true;
            gpu.supportsVulkan = true;

            // Parse memory (format: "xxxx MiB")
            QString memStr = parts[1].trimmed();
            memStr.remove("MiB").remove("MB");
            gpu.totalMemoryMB = memStr.trimmed().toULongLong();

            if (parts.count() >= 3) {
                gpu.driverVersion = parts[2].trimmed();
            }

            // Estimate compute capability and CUDA cores
            // This is simplified - would query via CUDA API in production
            if (gpu.name.contains("RTX 40")) {
                gpu.computeCapability = 89; // Ada Lovelace
                gpu.cudaCores = 16384;
            } else if (gpu.name.contains("RTX 30")) {
                gpu.computeCapability = 86; // Ampere
                gpu.cudaCores = 10496;
            } else if (gpu.name.contains("RTX 20") || gpu.name.contains("GTX 16")) {
                gpu.computeCapability = 75; // Turing
                gpu.cudaCores = 4608;
            } else if (gpu.name.contains("GTX 10")) {
                gpu.computeCapability = 61; // Pascal
                gpu.cudaCores = 3584;
            } else {
                gpu.computeCapability = 50; // Maxwell or older
                gpu.cudaCores = 2048;
            }

            gpu.freeMemoryMB = getGPUFreeMemory(i);

            gpus.append(gpu);
        }
    }

    return gpus;
}

QList<GPUInfo> GPUDetector::detectAMDGPUs()
{
    QList<GPUInfo> gpus;

    // Simplified - would use ROCm or OpenCL queries in production
    QString output = executeCommand("lspci | grep -i vga | grep -i amd");

    if (!output.isEmpty()) {
        GPUInfo gpu;
        gpu.name = "AMD GPU (detected via lspci)";
        gpu.type = GPUType::AMD_OpenCL;
        gpu.deviceId = 0;
        gpu.totalMemoryMB = 4096; // Default estimate
        gpu.freeMemoryMB = 3072;
        gpu.supportsCUDA = false;
        gpu.supportsOpenCL = true;
        gpu.supportsVulkan = true;

        gpus.append(gpu);
    }

    return gpus;
}

QList<GPUInfo> GPUDetector::detectIntelGPUs()
{
    QList<GPUInfo> gpus;

    // Detect Intel integrated graphics
    QString output = executeCommand("lspci | grep -i vga | grep -i intel");

    if (!output.isEmpty()) {
        GPUInfo gpu;
        gpu.name = "Intel Integrated Graphics";
        gpu.type = GPUType::Intel;
        gpu.deviceId = 0;
        gpu.totalMemoryMB = 2048; // Shared memory estimate
        gpu.freeMemoryMB = 1536;
        gpu.supportsCUDA = false;
        gpu.supportsOpenCL = true;
        gpu.supportsVulkan = true;

        gpus.append(gpu);
    }

    return gpus;
}

GPUInfo GPUDetector::parseNvidiaSMIOutput(const QString& output)
{
    Q_UNUSED(output);
    return GPUInfo();
}

GPUInfo GPUDetector::parseLspciOutput(const QString& output)
{
    Q_UNUSED(output);
    return GPUInfo();
}

QString GPUDetector::executeCommand(const QString& command)
{
    QProcess process;
    process.start("/bin/sh", QStringList() << "-c" << command);
    process.waitForFinished(3000); // 3 second timeout

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        return QString::fromUtf8(process.readAllStandardOutput());
    }

    return QString();
}

int GPUDetector::selectBestGPU(const QList<GPUInfo>& gpus)
{
    if (gpus.isEmpty()) {
        return -1;
    }

    int bestIdx = 0;
    double bestScore = 0.0;

    for (int i = 0; i < gpus.count(); ++i) {
        double score = calculatePerformanceScore(gpus[i]);
        if (score > bestScore) {
            bestScore = score;
            bestIdx = i;
        }
    }

    return bestIdx;
}

double GPUDetector::calculatePerformanceScore(const GPUInfo& gpu)
{
    double score = 0.0;

    // Memory contribution (40%)
    score += (gpu.totalMemoryMB / 16384.0) * 40.0; // Normalize to 16GB

    // CUDA cores contribution (30%)
    if (gpu.cudaCores > 0) {
        score += (gpu.cudaCores / 10000.0) * 30.0; // Normalize to 10k cores
    }

    // Compute capability contribution (20%)
    if (gpu.computeCapability > 0) {
        score += (gpu.computeCapability / 100.0) * 20.0; // Normalize
    }

    // Backend support (10%)
    if (gpu.supportsCUDA) {
        score += 10.0;
    } else if (gpu.supportsOpenCL) {
        score += 5.0;
    }

    return std::min(score, 100.0);
}

} // namespace Photogrammetry
} // namespace DroneMapper
