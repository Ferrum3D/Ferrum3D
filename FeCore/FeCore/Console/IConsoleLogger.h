#pragma once
#include <FeCore/Console/Console.h>
#include <FeCore/Strings/Format.h>

namespace FE::Debug
{
    using FE::Console::Color;

    // clang-format off
    /**
    * @brief Type of log message
    */
    FE_ENUM(LogMessageType)
    {
        /**
         * @brief No flags
        */
        None = 0,

        /**
        * @brief A regular debug message
        */
        Message = 1 << 0,

        /**
        * @brief Warning
        */
        Warning = 1 << 1,

        /**
        * @brief Error
        */
        Error = 1 << 2
    };
    // clang-format on

    class IConsoleLogger
    {
    protected:
        virtual void PrintImpl(StringSlice message)                     = 0;
        virtual void ColoredPrintImpl(Color color, StringSlice message) = 0;
        virtual void LogImpl(LogMessageType type, StringSlice message)  = 0;

    public:
        virtual void SetDebugLevel(LogMessageType types) = 0;

        template<class... Args>
        FE_FINLINE void Print(StringSlice fmt, Args&&... args)
        {
            PrintImpl(FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        template<class... Args>
        FE_FINLINE void ColoredPrint(Color color, StringSlice fmt, Args&&... args)
        {
            ColoredPrintImpl(color, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        template<class... Args>
        FE_FINLINE void Log(LogMessageType type, StringSlice fmt, Args&&... args)
        {
            LogImpl(type, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        template<class... Args>
        FE_FINLINE void LogError(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Error, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        template<class... Args>
        FE_FINLINE void LogWarning(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Warning, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        template<class... Args>
        FE_FINLINE void LogMessage(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Message, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }
    };
} // namespace FE::Debug
