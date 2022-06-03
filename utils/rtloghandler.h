#ifndef RTLOGHANDLER_H
#define RTLOGHANDLER_H

#include <QVariant>
#include <QString>
#include <QMutex>
#include "utils/callcontext.h"

/// Real-Time Log-Handler Utility
/// \note Use `#define RTLOGHANDLER_FORMAT "<custom-format>"`
///     in order to have a custom formatted logging.
///     Available Arguments:
///         > "%type": log-type (DEBUG, WARNING, ...)
///         > "%datetime": iso formatted date-time
///         > "%file": __FILE__
///         > "%line": __LINE__
///         > "%fnc": __PRETTY_FUNCTION__
///         > "%msg": log message
/// \note Use `#define RTLOGHANDLER_LOGFILE "<custom-path>"`
///     in order to set a custom log-file path.
/// \example Usage example:
///     `RT_DEBUG("message: %1, %2") % arg1 % arg2;`
class RTLogHandler
{
// public definitions
public:
    /// Enumerator for Log-Types
    enum Type { Debug, Warning, Critical, Fatal };

// construction
public:
    /// Basic Construction giving execution context
    RTLogHandler(const CallContext &context, Type type, const QString &msg);
    /// Dtor
    ~RTLogHandler();

// public interface
public:
    /// Templated argument-replacing operator
    template <typename T> RTLogHandler& operator%(const T& arg)
        { return this->operator%(QVariant::fromValue(arg).toString()); }
    /// Argument-replacing specialization for QVariant
    RTLogHandler& operator%(const QVariant& arg);
    /// Argument-replacing specialization for QString
    RTLogHandler& operator%(const QString& arg);
    /// Argument-replacing specialization for const char *
    RTLogHandler& operator%(const char *arg);


// private members
private:
    // Thread-Safety Mutex
    static QMutex mutex;
    // Call Context
    CallContext context;
    // Log-Type
    Type type;
    // Final Output Message
    QString msg;
    // Last Replaced Index
    int index = 0;
};

/// Some useful MACROS to handle LOGGING
#define RT_DEBUG(msg) \
    RTLogHandler(CALL_CONTEXT, RTLogHandler::Debug, msg)
#define RT_WARNING(msg) \
    RTLogHandler(CALL_CONTEXT, RTLogHandler::Warning, msg)
#define RT_CRITICAL(msg) \
    RTLogHandler(CALL_CONTEXT, RTLogHandler::Critical, msg)
#define RT_FATAL(msg) \
    RTLogHandler(CALL_CONTEXT, RTLogHandler::Fatal, msg)

/// Some useful MACROS to handle ASSERTIONS
#define RT_CHECK(cond) if(Q_UNLIKELY(!(cond))) RT_FATAL(#cond " condition failed!")

#endif // RTLOGHANDLER_H
