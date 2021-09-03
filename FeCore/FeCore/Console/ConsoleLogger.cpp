#include <FeCore/Console/ConsoleLogger.h>
#include <FeCore/Time/DateTime.h>

namespace FE::Debug
{
    ConsoleLogger::ConsoleLogger()
    {
        FE::Console::Init();
        FE::Console::ResetColor();
    }

    void ConsoleLogger::PrintImpl(StringSlice message)
    {
        FE::Console::PrintToStdout(message);
    }

    void ConsoleLogger::ColoredPrintImpl(Color color, StringSlice message)
    {
        FE::Console::SetColor(color);
        PrintImpl(message);
        FE::Console::ResetColor();
    }

    void ConsoleLogger::LogImpl(LogMessageType type, StringSlice message)
    {
        if ((type & m_DebugLevel) == LogMessageType::None)
            return;

        UniqueLocker lk(FE::Console::StdoutMutex);
        auto date = FE::DateTime::Now().ToString() + m_Header;
        PrintImpl(date);

        switch (type)
        {
        case LogMessageType::Message:
            ColoredPrintImpl(Color::Blue, "message");
            break;
        case LogMessageType::Warning:
            ColoredPrintImpl(Color::Yellow, "warning");
            break;
        case LogMessageType::Error:
            ColoredPrintImpl(Color::Red, "error");
            break;
        default:
            break;
        }

        PrintImpl(FE::String("]: ") + message + "\n");
    }

    void ConsoleLogger::SetDebugLevel(LogMessageType types)
    {
        m_DebugLevel = types;
    }
} // namespace FE::Debug
