#ifndef PROCESSINGQUEUE_H
#define PROCESSINGQUEUE_H

#include <QString>
#include <QList>
#include <QDateTime>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

namespace DroneMapper {
namespace Photogrammetry {

/**
 * @brief Job status
 */
enum class JobStatus {
    Queued,         // Waiting to be processed
    Running,        // Currently processing
    Paused,         // Temporarily paused
    Completed,      // Successfully completed
    Failed,         // Failed with error
    Cancelled       // Cancelled by user
};

/**
 * @brief Job priority levels
 */
enum class JobPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Processing job definition
 */
struct ProcessingJob {
    QString id;
    QString name;
    QString type;               // "alignment", "dense_cloud", "mesh", etc.
    JobStatus status;
    JobPriority priority;

    QString inputPath;
    QString outputPath;
    QStringList inputFiles;

    QDateTime queuedTime;
    QDateTime startTime;
    QDateTime completedTime;

    double progress;            // 0.0 - 100.0
    QString currentStep;
    QString errorMessage;

    int retryCount;
    int maxRetries;

    bool useGPU;
    int gpuDeviceId;

    qint64 estimatedTimeSeconds;
    qint64 elapsedTimeSeconds;

    QString getStatusString() const;
    QString getPriorityString() const;
    double getProgressPercent() const { return progress; }
    QString getElapsedTimeString() const;
    QString getETAString() const;
};

/**
 * @brief Queue statistics
 */
struct QueueStats {
    int totalJobs;
    int queuedJobs;
    int runningJobs;
    int completedJobs;
    int failedJobs;
    int cancelledJobs;

    double avgProcessingTime;   // Seconds
    double totalProcessingTime; // Seconds

    QString getSummary() const;
};

/**
 * @brief Job worker thread
 */
class JobWorker : public QThread {
    Q_OBJECT

public:
    JobWorker(QObject* parent = nullptr);
    ~JobWorker() override;

    void assignJob(const ProcessingJob& job);
    void stop();
    void pause();
    void resume();

signals:
    void jobStarted(const QString& jobId);
    void jobProgress(const QString& jobId, double progress, const QString& step);
    void jobCompleted(const QString& jobId);
    void jobFailed(const QString& jobId, const QString& error);

protected:
    void run() override;

private:
    ProcessingJob m_currentJob;
    bool m_stopRequested;
    bool m_paused;
    QMutex m_mutex;
    QWaitCondition m_condition;

    void processJob();
    bool executeJobStep(const QString& step);
};

/**
 * @brief Manages photogrammetry processing job queue
 *
 * Features:
 * - Multi-threaded job processing
 * - Priority queue management
 * - Progress tracking
 * - Job retry on failure
 * - GPU/CPU resource allocation
 * - Concurrent job limits
 * - Job dependencies
 * - Pause/resume capability
 * - Detailed logging
 *
 * Supported job types:
 * - Image alignment
 * - Dense point cloud generation
 * - Mesh reconstruction
 * - Texture mapping
 * - Orthomosaic generation
 * - Export operations
 */
class ProcessingQueue : public QObject {
    Q_OBJECT

public:
    explicit ProcessingQueue(QObject* parent = nullptr);
    ~ProcessingQueue() override;

    /**
     * @brief Add job to queue
     * @param job Job to add
     * @return Job ID
     */
    QString addJob(const ProcessingJob& job);

    /**
     * @brief Remove job from queue
     * @param jobId Job ID to remove
     * @return True if removed
     */
    bool removeJob(const QString& jobId);

    /**
     * @brief Cancel running job
     * @param jobId Job ID to cancel
     * @return True if cancelled
     */
    bool cancelJob(const QString& jobId);

    /**
     * @brief Pause job
     * @param jobId Job ID to pause
     * @return True if paused
     */
    bool pauseJob(const QString& jobId);

    /**
     * @brief Resume job
     * @param jobId Job ID to resume
     * @return True if resumed
     */
    bool resumeJob(const QString& jobId);

    /**
     * @brief Retry failed job
     * @param jobId Job ID to retry
     * @return True if retried
     */
    bool retryJob(const QString& jobId);

    /**
     * @brief Get job by ID
     * @param jobId Job ID
     * @return Job (empty if not found)
     */
    ProcessingJob getJob(const QString& jobId) const;

    /**
     * @brief Get all jobs
     * @return List of all jobs
     */
    QList<ProcessingJob> getAllJobs() const;

    /**
     * @brief Get jobs by status
     * @param status Status filter
     * @return List of matching jobs
     */
    QList<ProcessingJob> getJobsByStatus(JobStatus status) const;

    /**
     * @brief Get queue statistics
     * @return Queue stats
     */
    QueueStats getStatistics() const;

    /**
     * @brief Set maximum concurrent jobs
     * @param maxJobs Maximum concurrent jobs
     */
    void setMaxConcurrentJobs(int maxJobs);

    /**
     * @brief Get maximum concurrent jobs
     * @return Max concurrent jobs
     */
    int maxConcurrentJobs() const { return m_maxConcurrentJobs; }

    /**
     * @brief Clear all completed jobs
     */
    void clearCompletedJobs();

    /**
     * @brief Clear all jobs (stop processing)
     */
    void clearAllJobs();

    /**
     * @brief Start queue processing
     */
    void start();

    /**
     * @brief Stop queue processing
     */
    void stop();

    /**
     * @brief Pause all processing
     */
    void pauseAll();

    /**
     * @brief Resume all processing
     */
    void resumeAll();

    /**
     * @brief Check if queue is running
     * @return True if running
     */
    bool isRunning() const { return m_running; }

signals:
    void jobAdded(const QString& jobId);
    void jobStarted(const QString& jobId);
    void jobProgress(const QString& jobId, double progress, const QString& step);
    void jobCompleted(const QString& jobId);
    void jobFailed(const QString& jobId, const QString& error);
    void jobCancelled(const QString& jobId);
    void queueEmpty();

private slots:
    void onWorkerJobStarted(const QString& jobId);
    void onWorkerJobProgress(const QString& jobId, double progress, const QString& step);
    void onWorkerJobCompleted(const QString& jobId);
    void onWorkerJobFailed(const QString& jobId, const QString& error);

private:
    QList<ProcessingJob> m_jobs;
    QList<JobWorker*> m_workers;

    int m_maxConcurrentJobs;
    bool m_running;

    mutable QMutex m_mutex;

    QString generateJobId() const;
    void processNextJobs();
    ProcessingJob* findJob(const QString& jobId);
    const ProcessingJob* findJob(const QString& jobId) const;
    int getRunningJobCount() const;
    ProcessingJob* getNextJobToProcess();
    void updateJobStatus(const QString& jobId, JobStatus status);
};

} // namespace Photogrammetry
} // namespace DroneMapper

#endif // PROCESSINGQUEUE_H
