#include "ProcessingQueue.h"
#include <QUuid>
#include <QElapsedTimer>
#include <algorithm>

namespace DroneMapper {
namespace Photogrammetry {

QString ProcessingJob::getStatusString() const
{
    switch (status) {
    case JobStatus::Queued:     return "Queued";
    case JobStatus::Running:    return "Running";
    case JobStatus::Paused:     return "Paused";
    case JobStatus::Completed:  return "Completed";
    case JobStatus::Failed:     return "Failed";
    case JobStatus::Cancelled:  return "Cancelled";
    }
    return "Unknown";
}

QString ProcessingJob::getPriorityString() const
{
    switch (priority) {
    case JobPriority::Low:      return "Low";
    case JobPriority::Normal:   return "Normal";
    case JobPriority::High:     return "High";
    case JobPriority::Critical: return "Critical";
    }
    return "Unknown";
}

QString ProcessingJob::getElapsedTimeString() const
{
    qint64 seconds = elapsedTimeSeconds;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

QString ProcessingJob::getETAString() const
{
    if (progress <= 0.0 || elapsedTimeSeconds <= 0) {
        return "Unknown";
    }

    qint64 estimatedTotal = (elapsedTimeSeconds * 100.0) / progress;
    qint64 remaining = estimatedTotal - elapsedTimeSeconds;

    if (remaining < 60) {
        return QString("%1 seconds").arg(remaining);
    } else if (remaining < 3600) {
        return QString("%1 minutes").arg(remaining / 60);
    } else {
        return QString("%1 hours %2 minutes")
            .arg(remaining / 3600)
            .arg((remaining % 3600) / 60);
    }
}

QString QueueStats::getSummary() const
{
    return QString("Total: %1, Queued: %2, Running: %3, Completed: %4, Failed: %5")
        .arg(totalJobs)
        .arg(queuedJobs)
        .arg(runningJobs)
        .arg(completedJobs)
        .arg(failedJobs);
}

// JobWorker implementation

JobWorker::JobWorker(QObject* parent)
    : QThread(parent)
    , m_stopRequested(false)
    , m_paused(false)
{
}

JobWorker::~JobWorker()
{
    stop();
    wait();
}

void JobWorker::assignJob(const ProcessingJob& job)
{
    QMutexLocker locker(&m_mutex);
    m_currentJob = job;
    m_currentJob.status = JobStatus::Running;
    m_currentJob.startTime = QDateTime::currentDateTime();
}

void JobWorker::stop()
{
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
    m_condition.wakeOne();
}

void JobWorker::pause()
{
    QMutexLocker locker(&m_mutex);
    m_paused = true;
}

void JobWorker::resume()
{
    QMutexLocker locker(&m_mutex);
    m_paused = false;
    m_condition.wakeOne();
}

void JobWorker::run()
{
    emit jobStarted(m_currentJob.id);

    try {
        processJob();

        if (!m_stopRequested) {
            m_currentJob.status = JobStatus::Completed;
            m_currentJob.completedTime = QDateTime::currentDateTime();
            m_currentJob.progress = 100.0;
            emit jobCompleted(m_currentJob.id);
        }
    } catch (const std::exception& e) {
        m_currentJob.status = JobStatus::Failed;
        m_currentJob.errorMessage = QString::fromStdString(e.what());
        emit jobFailed(m_currentJob.id, m_currentJob.errorMessage);
    } catch (...) {
        m_currentJob.status = JobStatus::Failed;
        m_currentJob.errorMessage = "Unknown error occurred";
        emit jobFailed(m_currentJob.id, m_currentJob.errorMessage);
    }
}

void JobWorker::processJob()
{
    QElapsedTimer timer;
    timer.start();

    // Simulate multi-step processing
    QStringList steps;
    steps << "Initializing" << "Loading data" << "Processing" << "Finalizing";

    for (int i = 0; i < steps.count(); ++i) {
        // Check for pause
        while (m_paused && !m_stopRequested) {
            QMutexLocker locker(&m_mutex);
            m_condition.wait(&m_mutex, 100);
        }

        if (m_stopRequested) {
            m_currentJob.status = JobStatus::Cancelled;
            return;
        }

        QString step = steps[i];
        m_currentJob.currentStep = step;

        emit jobProgress(m_currentJob.id, (i * 100.0) / steps.count(), step);

        // Simulate processing time
        if (!executeJobStep(step)) {
            throw std::runtime_error(QString("Failed at step: %1").arg(step).toStdString());
        }

        m_currentJob.elapsedTimeSeconds = timer.elapsed() / 1000;
    }

    m_currentJob.progress = 100.0;
}

bool JobWorker::executeJobStep(const QString& step)
{
    Q_UNUSED(step);

    // Simplified - actual implementation would call photogrammetry pipeline
    // For now, just simulate work
    QThread::msleep(100);

    return true;
}

// ProcessingQueue implementation

ProcessingQueue::ProcessingQueue(QObject* parent)
    : QObject(parent)
    , m_maxConcurrentJobs(4)
    , m_running(false)
{
}

ProcessingQueue::~ProcessingQueue()
{
    stop();
    clearAllJobs();
}

QString ProcessingQueue::addJob(const ProcessingJob& job)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob newJob = job;
    newJob.id = generateJobId();
    newJob.status = JobStatus::Queued;
    newJob.queuedTime = QDateTime::currentDateTime();
    newJob.progress = 0.0;
    newJob.retryCount = 0;

    if (newJob.maxRetries == 0) {
        newJob.maxRetries = 3; // Default
    }

    m_jobs.append(newJob);

    emit jobAdded(newJob.id);

    if (m_running) {
        locker.unlock();
        processNextJobs();
    }

    return newJob.id;
}

bool ProcessingQueue::removeJob(const QString& jobId)
{
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_jobs.count(); ++i) {
        if (m_jobs[i].id == jobId) {
            if (m_jobs[i].status == JobStatus::Running) {
                // Can't remove running job
                return false;
            }

            m_jobs.removeAt(i);
            return true;
        }
    }

    return false;
}

bool ProcessingQueue::cancelJob(const QString& jobId)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob* job = findJob(jobId);
    if (!job) return false;

    if (job->status == JobStatus::Running) {
        // Find worker and stop it
        for (auto* worker : m_workers) {
            worker->stop();
        }
    }

    job->status = JobStatus::Cancelled;
    emit jobCancelled(jobId);

    return true;
}

bool ProcessingQueue::pauseJob(const QString& jobId)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob* job = findJob(jobId);
    if (!job) return false;

    if (job->status != JobStatus::Running) {
        return false;
    }

    job->status = JobStatus::Paused;

    // Pause worker
    for (auto* worker : m_workers) {
        worker->pause();
    }

    return true;
}

bool ProcessingQueue::resumeJob(const QString& jobId)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob* job = findJob(jobId);
    if (!job) return false;

    if (job->status != JobStatus::Paused) {
        return false;
    }

    job->status = JobStatus::Running;

    // Resume worker
    for (auto* worker : m_workers) {
        worker->resume();
    }

    return true;
}

bool ProcessingQueue::retryJob(const QString& jobId)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob* job = findJob(jobId);
    if (!job) return false;

    if (job->status != JobStatus::Failed) {
        return false;
    }

    if (job->retryCount >= job->maxRetries) {
        return false; // Max retries exceeded
    }

    job->status = JobStatus::Queued;
    job->progress = 0.0;
    job->errorMessage.clear();
    job->retryCount++;

    if (m_running) {
        locker.unlock();
        processNextJobs();
    }

    return true;
}

ProcessingJob ProcessingQueue::getJob(const QString& jobId) const
{
    QMutexLocker locker(&m_mutex);

    const ProcessingJob* job = findJob(jobId);
    if (job) {
        return *job;
    }

    return ProcessingJob();
}

QList<ProcessingJob> ProcessingQueue::getAllJobs() const
{
    QMutexLocker locker(&m_mutex);
    return m_jobs;
}

QList<ProcessingJob> ProcessingQueue::getJobsByStatus(JobStatus status) const
{
    QMutexLocker locker(&m_mutex);

    QList<ProcessingJob> filtered;
    for (const auto& job : m_jobs) {
        if (job.status == status) {
            filtered.append(job);
        }
    }

    return filtered;
}

QueueStats ProcessingQueue::getStatistics() const
{
    QMutexLocker locker(&m_mutex);

    QueueStats stats;
    stats.totalJobs = m_jobs.count();
    stats.queuedJobs = 0;
    stats.runningJobs = 0;
    stats.completedJobs = 0;
    stats.failedJobs = 0;
    stats.cancelledJobs = 0;
    stats.totalProcessingTime = 0.0;

    for (const auto& job : m_jobs) {
        switch (job.status) {
        case JobStatus::Queued:     stats.queuedJobs++; break;
        case JobStatus::Running:    stats.runningJobs++; break;
        case JobStatus::Completed:  stats.completedJobs++; break;
        case JobStatus::Failed:     stats.failedJobs++; break;
        case JobStatus::Cancelled:  stats.cancelledJobs++; break;
        default: break;
        }

        if (job.status == JobStatus::Completed) {
            stats.totalProcessingTime += job.elapsedTimeSeconds;
        }
    }

    stats.avgProcessingTime = stats.completedJobs > 0 ?
        stats.totalProcessingTime / stats.completedJobs : 0.0;

    return stats;
}

void ProcessingQueue::setMaxConcurrentJobs(int maxJobs)
{
    QMutexLocker locker(&m_mutex);
    m_maxConcurrentJobs = std::max(1, maxJobs);
}

void ProcessingQueue::clearCompletedJobs()
{
    QMutexLocker locker(&m_mutex);

    for (int i = m_jobs.count() - 1; i >= 0; --i) {
        if (m_jobs[i].status == JobStatus::Completed) {
            m_jobs.removeAt(i);
        }
    }
}

void ProcessingQueue::clearAllJobs()
{
    stop();

    QMutexLocker locker(&m_mutex);
    m_jobs.clear();
}

void ProcessingQueue::start()
{
    QMutexLocker locker(&m_mutex);

    if (m_running) return;

    m_running = true;
    locker.unlock();

    processNextJobs();
}

void ProcessingQueue::stop()
{
    QMutexLocker locker(&m_mutex);

    if (!m_running) return;

    m_running = false;

    // Stop all workers
    for (auto* worker : m_workers) {
        worker->stop();
        worker->wait();
        delete worker;
    }
    m_workers.clear();
}

void ProcessingQueue::pauseAll()
{
    QMutexLocker locker(&m_mutex);

    for (auto* worker : m_workers) {
        worker->pause();
    }

    for (auto& job : m_jobs) {
        if (job.status == JobStatus::Running) {
            job.status = JobStatus::Paused;
        }
    }
}

void ProcessingQueue::resumeAll()
{
    QMutexLocker locker(&m_mutex);

    for (auto* worker : m_workers) {
        worker->resume();
    }

    for (auto& job : m_jobs) {
        if (job.status == JobStatus::Paused) {
            job.status = JobStatus::Running;
        }
    }
}

void ProcessingQueue::onWorkerJobStarted(const QString& jobId)
{
    updateJobStatus(jobId, JobStatus::Running);
    emit jobStarted(jobId);
}

void ProcessingQueue::onWorkerJobProgress(const QString& jobId, double progress, const QString& step)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob* job = findJob(jobId);
    if (job) {
        job->progress = progress;
        job->currentStep = step;
    }

    emit jobProgress(jobId, progress, step);
}

void ProcessingQueue::onWorkerJobCompleted(const QString& jobId)
{
    updateJobStatus(jobId, JobStatus::Completed);
    emit jobCompleted(jobId);

    processNextJobs();
}

void ProcessingQueue::onWorkerJobFailed(const QString& jobId, const QString& error)
{
    {
        QMutexLocker locker(&m_mutex);

        ProcessingJob* job = findJob(jobId);
        if (job) {
            job->status = JobStatus::Failed;
            job->errorMessage = error;
        }
    }

    emit jobFailed(jobId, error);

    processNextJobs();
}

QString ProcessingQueue::generateJobId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void ProcessingQueue::processNextJobs()
{
    QMutexLocker locker(&m_mutex);

    if (!m_running) return;

    int runningCount = getRunningJobCount();

    while (runningCount < m_maxConcurrentJobs) {
        ProcessingJob* nextJob = getNextJobToProcess();

        if (!nextJob) {
            if (runningCount == 0 && getJobsByStatus(JobStatus::Queued).isEmpty()) {
                emit queueEmpty();
            }
            break;
        }

        // Create worker and start job
        JobWorker* worker = new JobWorker(this);
        connect(worker, &JobWorker::jobStarted, this, &ProcessingQueue::onWorkerJobStarted);
        connect(worker, &JobWorker::jobProgress, this, &ProcessingQueue::onWorkerJobProgress);
        connect(worker, &JobWorker::jobCompleted, this, &ProcessingQueue::onWorkerJobCompleted);
        connect(worker, &JobWorker::jobFailed, this, &ProcessingQueue::onWorkerJobFailed);

        worker->assignJob(*nextJob);
        worker->start();

        m_workers.append(worker);
        runningCount++;
    }
}

ProcessingJob* ProcessingQueue::findJob(const QString& jobId)
{
    for (auto& job : m_jobs) {
        if (job.id == jobId) {
            return &job;
        }
    }
    return nullptr;
}

const ProcessingJob* ProcessingQueue::findJob(const QString& jobId) const
{
    for (const auto& job : m_jobs) {
        if (job.id == jobId) {
            return &job;
        }
    }
    return nullptr;
}

int ProcessingQueue::getRunningJobCount() const
{
    int count = 0;
    for (const auto& job : m_jobs) {
        if (job.status == JobStatus::Running) {
            count++;
        }
    }
    return count;
}

ProcessingJob* ProcessingQueue::getNextJobToProcess()
{
    // Find highest priority queued job
    ProcessingJob* best = nullptr;
    int bestPriority = -1;

    for (auto& job : m_jobs) {
        if (job.status == JobStatus::Queued) {
            int priority = static_cast<int>(job.priority);
            if (priority > bestPriority) {
                bestPriority = priority;
                best = &job;
            }
        }
    }

    return best;
}

void ProcessingQueue::updateJobStatus(const QString& jobId, JobStatus status)
{
    QMutexLocker locker(&m_mutex);

    ProcessingJob* job = findJob(jobId);
    if (job) {
        job->status = status;
    }
}

} // namespace Photogrammetry
} // namespace DroneMapper
