#pragma once
#include <Core/Memory/Memory.h>
#include <Core/Strings/Format.h>
#include <Core/Threading/SpinLock.h>
#include <festd/intrusive_list.h>

namespace FE::Logger
{
    //! @brief Log message severity.
    enum class Severity
    {
        kTrace = 0,    //!< The most verbose messages disabled by default.
        kDebug = 1,    //!< Messages used for debugging in development builds.
        kInfo = 2,     //!< General information messages.
        kWarning = 3,  //!< Warning messages.
        kError = 4,    //!< Error messages that indicate local or recoverable failures.
        kCritical = 5, //!< Error messages that indicate critical and unrecoverable failures.
    };


    constexpr const char* LogSeverityToString(const Severity severity)
    {
        switch (severity)
        {
        default:
            return "<unknown>";

        case Severity::kTrace:
            return "trace";
        case Severity::kDebug:
            return "debug";
        case Severity::kInfo:
            return "info";
        case Severity::kWarning:
            return "warning";
        case Severity::kError:
            return "error";
        case Severity::kCritical:
            return "critical";
        }
    }


    enum class SeverityFlags
    {
        kTrace = 1 << festd::to_underlying(Severity::kTrace),       //!< See Severity::kTrace.
        kDebug = 1 << festd::to_underlying(Severity::kDebug),       //!< See Severity::kDebug.
        kInfo = 1 << festd::to_underlying(Severity::kInfo),         //!< See Severity::kInfo.
        kWarning = 1 << festd::to_underlying(Severity::kWarning),   //!< See Severity::kWarning.
        kError = 1 << festd::to_underlying(Severity::kError),       //!< See Severity::kError.
        kCritical = 1 << festd::to_underlying(Severity::kCritical), //!< See Severity::kCritical.

        kNone = 0,                                   //!< Value used to specify that nothing should be logged.
        kErrorsOnly = kError | kCritical,            //!< Value used to specify that only errors should be logged.
        kProduction = kWarning | kErrorsOnly,        //!< Value used to specify default production logging severity.
        kDevelopment = kInfo | kDebug | kProduction, //!< Value used to specify default development logging severity.
        kAll = kTrace | kDevelopment,                //!< Value used to specify that all messages should be logged.
    };

    FE_ENUM_OPERATORS(SeverityFlags);


    struct SinkBase : public festd::intrusive_list_node
    {
        virtual ~SinkBase();
        virtual void Log(Severity severity, SourceLocation sourceLocation, festd::string_view message) = 0;

    protected:
        SinkBase();
    };


    void LogImpl(Severity severity, SourceLocation sourceLocation, festd::string_view message);


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


    template<class... TArgs>
    void Log(const Severity severity, LogFormatString fmt, TArgs&&... args)
    {
        festd::inline_string message;
        Fmt::FormatTo(message, fmt.m_value, std::forward<TArgs>(args)...);
        LogImpl(severity, fmt.m_location, message);
    }


    template<class... TArgs>
    void LogTrace(const LogFormatString fmt, TArgs&&... args)
    {
        Log(Severity::kTrace, fmt, std::forward<TArgs>(args)...);
    }

    template<class... TArgs>
    void LogDebug(const LogFormatString fmt, TArgs&&... args)
    {
        Log(Severity::kDebug, fmt, std::forward<TArgs>(args)...);
    }

    template<class... TArgs>
    void LogInfo(const LogFormatString fmt, TArgs&&... args)
    {
        Log(Severity::kInfo, fmt, std::forward<TArgs>(args)...);
    }

    template<class... TArgs>
    void LogWarning(const LogFormatString fmt, TArgs&&... args)
    {
        Log(Severity::kWarning, fmt, std::forward<TArgs>(args)...);
    }

    template<class... TArgs>
    void LogError(const LogFormatString fmt, TArgs&&... args)
    {
        Log(Severity::kError, fmt, std::forward<TArgs>(args)...);
    }

    template<class... TArgs>
    void LogCritical(const LogFormatString fmt, TArgs&&... args)
    {
        Log(Severity::kCritical, fmt, std::forward<TArgs>(args)...);
    }
} // namespace FE::Logger
