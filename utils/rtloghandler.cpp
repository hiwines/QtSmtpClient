#include "rtloghandler.h"

#include <QMutexLocker>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QDateTime>
#include <cstdio>
#include <cstdlib>

#ifndef RTLOGHANDLER_FORMAT
#define RTLOGHANDLER_FORMAT "[%type] > [%datetime] [%file:%line] - %msg\n"
#endif

#ifndef RTLOGHANDLER_LOGFILE
#define RTLOGHANDLER_LOGFILE "application-log.out"
#endif

// PRIVATE UTILITY NAMESPACE
namespace {

/// Gets the string-encoding for the given log-type
QString toString(RTLogHandler::Type type)
{
    switch (type) {
        case RTLogHandler::Debug: return "DEBUG";
        case RTLogHandler::Warning: return "WARNING";
        case RTLogHandler::Critical: return "CRITICAL";
        case RTLogHandler::Fatal: return "FATAL";
        default: return QString(); // fallback
    }
}

} // PRIVATE UTILITY NAMESPACE

/// Static Mutex Instance
QMutex RTLogHandler::mutex;

RTLogHandler::RTLogHandler(
    const CallContext& context, Type type, const QString& msg)
    : context(context), type(type), msg(msg)
{}

RTLogHandler::~RTLogHandler()
{
    // starts locking shared mutex
    QMutexLocker locker (&mutex);

    // and then starts computing the final message
    QString log (RTLOGHANDLER_FORMAT);
    log.replace("%type", toString(type));
    log.replace("%datetime", QDateTime::currentDateTime().toString(Qt::ISODate));
    log.replace("%file", context.file);
    log.replace("%line", QString::number(context.line));
    log.replace("%fnc", context.fnc);
    log.replace("%msg", msg);

    // then prints it to the standard error stream
    fprintf(stderr, "%s", qPrintable(log));
    fflush(stderr); // ensure it is flushed

    // handles log rotation in order to avoid more than 4MB of logs
    if (QFileInfo(RTLOGHANDLER_LOGFILE).size() > 2 * 1024 * 1024) {
        // removes the old backup first
        QFile(RTLOGHANDLER_LOGFILE ".bak").remove();
        // moves the existing log to the backup destination
        QFile(RTLOGHANDLER_LOGFILE).rename(RTLOGHANDLER_LOGFILE ".bak");
    }

    // and finally appends it to the log file
    QFile logfile (RTLOGHANDLER_LOGFILE);
    if (logfile.open(QIODevice::Append | QIODevice::Text)) {
        logfile.write(log.toUtf8());
        logfile.close();
    }

    // handles application abort for fatal error
    if (type == Fatal)
        std::abort();
}

RTLogHandler& RTLogHandler::operator%(const QVariant& arg)
{
    // handles QVariant translation for detailed content
    QString type = (arg.isValid())? arg.typeName() : "Invalid";
    QString log = QString("QVariant(%1, `%2`, null? %3)").arg(type,
        arg.toString(), (arg.isNull())? "true" : "false");
    // replace the next indexed argument with the given arg
    msg.replace('%' + QString::number(++index), log);
    // allows concatenations
    return *this;
}

RTLogHandler& RTLogHandler::operator%(const QString& arg)
{
    // replace the next indexed argument with the given arg
    msg.replace('%' + QString::number(++index), arg);
    // allows concatenations
    return *this;
}

RTLogHandler& RTLogHandler::operator%(const char *arg)
{
    // replace the next indexed argument with the given arg
    msg.replace('%' + QString::number(++index), arg);
    // allows concatenations
    return *this;
}
