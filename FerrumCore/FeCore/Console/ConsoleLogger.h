#pragma once
#include <FeCore/Console/IConsoleLogger.h>
#include <FeCore/Modules/Singleton.h>

namespace FE::Debug
{
    //! \brief Implementation of IConsoleLogger that prints messages to stdout.
    class ConsoleLogger final : public FE::SingletonImplBase<IConsoleLogger>
    {
        LogMessageType m_DebugLevel = LogMessageType::All;
        FE::String m_Header         = FE::String(" ") + FE::FerrumEngineName + " [";

    protected:
        //=========================================================================================
        // IConsoleLogger

        void PrintImpl(StringSlice message) override;
        void ColoredPrintImpl(Console::Color color, StringSlice message) override;
        void LogImpl(LogMessageType type, StringSlice message) override;

    public:
        void SetDebugLevel(LogMessageType types) override;
        //=========================================================================================

        FE_CLASS_RTTI(ConsoleLogger, "3C19F24F-9F51-4F16-BE4D-C1468D3EA6A4");

        ConsoleLogger();
    };
} // namespace FE::Debug