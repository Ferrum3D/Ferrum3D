#include <Core/IO/BaseIO.h>
#include <Core/Logging/Logger.h>
#include <Core/Logging/LoggerPrivate.h>
#include <Core/Time/DateTime.h>

namespace FE
{
    namespace
    {
        festd::string_view GetSeverityColor(const Logger::Severity severity)
        {
            switch (severity)
            {
                using enum Logger::Severity;

            default:
                FE_DebugBreak();
                [[fallthrough]];

            case kWarning:
                return "\033[33m";

            case kError:
            case kCritical:
                return "\033[31m";

            case kTrace:
                return "\033[35m";

            case kDebug:
                return "\033[34m";

            case kInfo:
                return "\033[36m";
            }
        }


        struct StdoutLogSink final : public Logger::SinkBase
        {
            void Log(const Logger::Severity severity, const SourceLocation sourceLocation, festd::string_view message) override
            {
                const auto dateStr = DateTime<TZ::Local>::Now().ToString(DateTimeFormat::kISO8601);
                const auto colorStr = GetSeverityColor(severity);
                const auto severityStr = LogSeverityToString(severity);

                IO::PrintLn("{}({}): {} [{}{}\033[0m] {}",
                            sourceLocation.m_fileName,
                            sourceLocation.m_lineNumber,
                            dateStr,
                            colorStr,
                            severityStr,
                            message);
                IO::Flush(IO::StandardDescriptor::kStdout);
            }
        };


        struct DefaultSinks final
        {
            StdoutLogSink m_stdoutSink;
        };


        struct LoggingState final
        {
            Threading::SpinLock m_lock;
            festd::intrusive_list<Logger::SinkBase> m_sinks;
            DefaultSinks* m_defaultSinks = nullptr;
        };

        LoggingState* GLoggingState;
    } // namespace


    void Logger::Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_Assert(GLoggingState == nullptr, "Logger already initialized");
        GLoggingState = Memory::New<LoggingState>(allocator);
        GLoggingState->m_defaultSinks = Memory::New<DefaultSinks>(allocator);
    }


    void Logger::Internal::Shutdown()
    {
        FE_Assert(GLoggingState != nullptr, "Logger not initialized");
        GLoggingState->m_defaultSinks->~DefaultSinks();
        GLoggingState->~LoggingState();
        GLoggingState = nullptr;
    }


    Logger::SinkBase::~SinkBase()
    {
        std::lock_guard lk{ GLoggingState->m_lock };
        festd::intrusive_list<>::remove(*this);
    }


    Logger::SinkBase::SinkBase()
    {
        std::lock_guard lk{ GLoggingState->m_lock };
        GLoggingState->m_sinks.push_back(*this);
    }


    void Logger::LogImpl(const Severity severity, const SourceLocation sourceLocation, festd::string_view message)
    {
        std::lock_guard lk{ GLoggingState->m_lock };
        for (SinkBase& sink : GLoggingState->m_sinks)
            sink.Log(severity, sourceLocation, message);
    }
} // namespace FE
