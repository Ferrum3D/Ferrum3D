#pragma once
#include <FeCore/Console/IConsoleLogger.h>
#include <FeCore/Modules/Singleton.h>

namespace FE::Debug
{
    /**
     * @brief Implementation of IConsoleLogger that prints messages to stdout.
    */
    class ConsoleLogger : public FE::SingletonImplBase<IConsoleLogger>
    {
        LogMessageType m_DebugLevel = LogMessageType::All;

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
