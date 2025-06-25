#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/Format.h>
#include <FeCore/Threading/SpinLock.h>
#include <festd/intrusive_list.h>

namespace FE
{
    //! @brief Log message severity.
    enum class LogSeverity
    {
        kTrace = 0,    //!< The most verbose messages disabled by default.
        kDebug = 1,    //!< Messages used for debugging in development builds.
        kInfo = 2,     //!< General information messages.
        kWarning = 3,  //!< Warning messages.
        kError = 4,    //!< Error messages that indicate local or recoverable failures.
        kCritical = 5, //!< Error messages that indicate critical and unrecoverable failures.
    };


    inline const char* LogSeverityToString(const LogSeverity severity)
    {
        switch (severity)
        {
        default:
            FE_DebugBreak();
            return "<unknown>";

        case LogSeverity::kTrace:
            return "trace";
        case LogSeverity::kDebug:
            return "debug";
        case LogSeverity::kInfo:
            return "info";
        case LogSeverity::kWarning:
            return "warning";
        case LogSeverity::kError:
            return "error";
        case LogSeverity::kCritical:
            return "critical";
        }
    }


    enum class LogSeverityFlags
    {
        kTrace = 1 << festd::to_underlying(LogSeverity::kTrace),       //!< See LogSeverity::kTrace.
        kDebug = 1 << festd::to_underlying(LogSeverity::kDebug),       //!< See LogSeverity::kDebug.
        kInfo = 1 << festd::to_underlying(LogSeverity::kInfo),         //!< See LogSeverity::kInfo.
        kWarning = 1 << festd::to_underlying(LogSeverity::kWarning),   //!< See LogSeverity::kWarning.
        kError = 1 << festd::to_underlying(LogSeverity::kError),       //!< See LogSeverity::kError.
        kCritical = 1 << festd::to_underlying(LogSeverity::kCritical), //!< See LogSeverity::kCritical.

        kNone = 0,                                   //!< Value used to specify that nothing should be logged.
        kErrorsOnly = kError | kCritical,            //!< Value used to specify that only errors should be logged.
        kProduction = kWarning | kErrorsOnly,        //!< Value used to specify default production logging severity.
        kDevelopment = kInfo | kDebug | kProduction, //!< Value used to specify default development logging severity.
        kAll = kTrace | kDevelopment,                //!< Value used to specify that all messages should be logged.
    };

    FE_ENUM_OPERATORS(LogSeverityFlags);


    struct Logger;

    struct LogSinkBase : public festd::intrusive_list_node
    {
        virtual ~LogSinkBase();

        virtual void Log(LogSeverity severity, SourceLocation sourceLocation, festd::string_view message) = 0;

    protected:
        Logger* m_pLogger;

        LogSinkBase(Logger* logger);
    };


    struct LogFormatString final
    {
        festd::string_view m_value;
        SourceLocation m_location;

        LogFormatString(const char* fmt, SourceLocation location = SourceLocation::Current())
            : m_value(fmt)
            , m_location(location)
        {
        }

        LogFormatString(festd::string_view fmt, SourceLocation location = SourceLocation::Current())
            : m_value(fmt)
            , m_location(location)
        {
        }
    };


    struct Logger final : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(Logger, "B54397F4-415F-4FA6-8124-4672D2A179CE");

        template<class... TArgs>
        void Log(LogSeverity severity, LogFormatString fmt, TArgs&&... args)
        {
            festd::string message;
            Fmt::FormatTo(message, fmt.m_value, std::forward<TArgs>(args)...);

            std::lock_guard lock{ m_lock };
            for (LogSinkBase& sink : m_sinks)
            {
                sink.Log(severity, fmt.m_location, message);
            }
        }

        template<class... TArgs>
        void LogTrace(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kTrace, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        void LogDebug(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kDebug, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        void LogInfo(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kInfo, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        void LogWarning(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kWarning, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        void LogError(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kError, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        void LogCritical(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kCritical, fmt, std::forward<TArgs>(args)...);
        }

    private:
        friend struct LogSinkBase;

        Threading::SpinLock m_lock;
        festd::intrusive_list<LogSinkBase> m_sinks;
    };
} // namespace FE
