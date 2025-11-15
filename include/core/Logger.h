#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

namespace DroneMapper {
namespace Core {

/**
 * @brief Application-wide logging system
 */
class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    static Logger& instance();

    void log(Level level, const QString& message, const QString& category = QString());
    void debug(const QString& message, const QString& category = QString());
    void info(const QString& message, const QString& category = QString());
    void warning(const QString& message, const QString& category = QString());
    void error(const QString& message, const QString& category = QString());
    void critical(const QString& message, const QString& category = QString());

    void setLogFile(const QString& filePath);
    void setLogLevel(Level level);

    QString logFilePath() const { return m_logFilePath; }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeToFile(const QString& message);

    QString m_logFilePath;
    Level m_minimumLevel;
    QFile m_logFile;
    QTextStream m_logStream;
    QMutex m_mutex;
};

} // namespace Core
} // namespace DroneMapper

// Convenience macros
#define LOG_DEBUG(msg) DroneMapper::Core::Logger::instance().debug(msg)
#define LOG_INFO(msg) DroneMapper::Core::Logger::instance().info(msg)
#define LOG_WARNING(msg) DroneMapper::Core::Logger::instance().warning(msg)
#define LOG_ERROR(msg) DroneMapper::Core::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) DroneMapper::Core::Logger::instance().critical(msg)

#endif // LOGGER_H
