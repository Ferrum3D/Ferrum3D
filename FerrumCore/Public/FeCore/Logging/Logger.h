#pragma once
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/Format.h>
#include <FeCore/Time/DateTime.h>

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


    inline constexpr const char* LogSeverityToString(LogSeverity severity)
    {
        switch (severity)
        {
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
        default:
            return "<unk>";
        }
    }


    enum class LogSeverityFlags
    {
        kTrace = 1 << enum_cast(LogSeverity::kTrace),       //!< See LogSeverity::kTrace.
        kDebug = 1 << enum_cast(LogSeverity::kDebug),       //!< See LogSeverity::kDebug.
        kInfo = 1 << enum_cast(LogSeverity::kInfo),         //!< See LogSeverity::kInfo.
        kWarning = 1 << enum_cast(LogSeverity::kWarning),   //!< See LogSeverity::kWarning.
        kError = 1 << enum_cast(LogSeverity::kError),       //!< See LogSeverity::kError.
        kCritical = 1 << enum_cast(LogSeverity::kCritical), //!< See LogSeverity::kCritical.

        kNone = 0,                                   //!< Value used to specify that nothing should be logged.
        kErrorsOnly = kError | kCritical,            //!< Value used to specify that only errors should be logged.
        kProduction = kWarning | kErrorsOnly,        //!< Value used to specify default production logging severity.
        kDevelopment = kInfo | kDebug | kProduction, //!< Value used to specify default development logging severity.
        kAll = kTrace | kDevelopment,                //!< Value used to specify that all messages should be logged.
    };

    FE_ENUM_OPERATORS(LogSeverityFlags);


    class Logger;

    class LogSinkBase : public festd::intrusive_list_node
    {
    protected:
        Logger* m_pLogger;

        LogSinkBase(Logger* logger);

    public:
        virtual ~LogSinkBase();

        virtual void Log(LogSeverity severity, SourceLocation sourceLocation, StringSlice message) = 0;
    };


    struct LogFormatString final
    {
        StringSlice Value;
        SourceLocation Location;

        inline LogFormatString(const char* fmt, SourceLocation location = SourceLocation::Current())
            : Value(fmt)
            , Location(location)
        {
        }

        inline LogFormatString(StringSlice fmt, SourceLocation location = SourceLocation::Current())
            : Value(fmt)
            , Location(location)
        {
        }
    };


    class Logger final : public Memory::RefCountedObjectBase
    {
        friend class LogSinkBase;

        SpinLock m_Lock;
        festd::intrusive_list<LogSinkBase> m_Sinks;

    public:
        FE_RTTI_Class(Logger, "B54397F4-415F-4FA6-8124-4672D2A179CE");

        template<class... TArgs>
        inline void Log(LogSeverity severity, LogFormatString fmt, TArgs&&... args)
        {
            String message;
            Fmt::FormatTo(message, fmt.Value, std::forward<TArgs>(args)...);

            std::lock_guard lock{ m_Lock };
            for (LogSinkBase& sink : m_Sinks)
            {
                sink.Log(severity, fmt.Location, message);
            }
        }

        template<class... TArgs>
        inline void LogTrace(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kTrace, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        inline void LogDebug(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kDebug, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        inline void LogInfo(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kInfo, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        inline void LogWarning(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kWarning, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        inline void LogError(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kError, fmt, std::forward<TArgs>(args)...);
        }

        template<class... TArgs>
        inline void LogCritical(LogFormatString fmt, TArgs&&... args)
        {
            Log(LogSeverity::kCritical, fmt, std::forward<TArgs>(args)...);
        }
    };
} // namespace FE
