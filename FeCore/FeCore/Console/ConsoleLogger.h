#pragma once
#include <Console/IConsoleLogger.h>
#include <Modules/Singleton.h>

namespace FE::Debug
{
    class ConsoleLogger : public FE::SingletonImplBase<IConsoleLogger>
    {
        LogMessageType m_DebugLevel = LogMessageType::Error | LogMessageType::Warning | LogMessageType::Message;

    protected:

        //=========================================================================================
        // IConsoleLogger

        virtual void PrintImpl(StringSlice message) override;
        virtual void ColoredPrintImpl(Console::Color color, StringSlice message) override;
        virtual void LogImpl(LogMessageType type, StringSlice message) override;

    public:
        virtual void SetDebugLevel(LogMessageType types) override;
        //=========================================================================================

        ConsoleLogger();
    };
} // namespace FE::Debug
