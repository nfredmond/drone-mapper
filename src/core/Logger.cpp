#include "Logger.h"
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QMutexLocker>

namespace DroneMapper {
namespace Core {

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_minimumLevel(Level::Info)
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);

    QString logPath = QDir(logDir).filePath("dronemapper.log");
    setLogFile(logPath);
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::setLogFile(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_logFile.close();
    }

    m_logFilePath = filePath;
    m_logFile.setFileName(filePath);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_logStream.setDevice(&m_logFile);
}

void Logger::setLogLevel(Level level)
{
    m_minimumLevel = level;
}

void Logger::log(Level level, const QString& message, const QString& category)
{
    if (level < m_minimumLevel) {
        return;
    }

    QString levelStr;
    switch (level) {
        case Level::Debug:    levelStr = "DEBUG"; break;
        case Level::Info:     levelStr = "INFO"; break;
        case Level::Warning:  levelStr = "WARN"; break;
        case Level::Error:    levelStr = "ERROR"; break;
        case Level::Critical: levelStr = "CRITICAL"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString categoryStr = category.isEmpty() ? "" : QString("[%1] ").arg(category);

    QString logMessage = QString("[%1] %2 %3%4")
        .arg(timestamp)
        .arg(levelStr)
        .arg(categoryStr)
        .arg(message);

    writeToFile(logMessage);
}

void Logger::debug(const QString& message, const QString& category)
{
    log(Level::Debug, message, category);
}

void Logger::info(const QString& message, const QString& category)
{
    log(Level::Info, message, category);
}

void Logger::warning(const QString& message, const QString& category)
{
    log(Level::Warning, message, category);
}

void Logger::error(const QString& message, const QString& category)
{
    log(Level::Error, message, category);
}

void Logger::critical(const QString& message, const QString& category)
{
    log(Level::Critical, message, category);
}

void Logger::writeToFile(const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_logStream << message << "\n";
        m_logStream.flush();
    }
}

} // namespace Core
} // namespace DroneMapper
