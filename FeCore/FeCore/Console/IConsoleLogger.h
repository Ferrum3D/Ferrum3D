#pragma once
#include <FeCore/Console/Console.h>
#include <FeCore/Strings/Format.h>

namespace FE::Debug
{
    using FE::Console::Color;

    // clang-format off
    /**
     * @brief Type of log message.
    */
    FE_ENUM(LogMessageType)
    {
        /**
         * @brief No flags.
        */
        None = 0,

        /**
         * @brief A regular debug message.
        */
        Message = 1 << 0,

        /**
         * @brief Warning.
        */
        Warning = 1 << 1,

        /**
         * @brief Error.
        */
        Error = 1 << 2,

        /**
         * @brief All messages
        */
        All = Message | Warning | Error
    };
    // clang-format on

    /**
     * @brief Interface that console loggers must implement.
     * 
     * All implementations must inherit from FE::SingletonImplBase<IConsoleLogger>, not the interface directly.
     * The loggers work with UTF-8 encoding and must set up their internal stream to use unicode.\n
     * \n
     * Usage:
     * FE::Singleton<FE::Debug::IConsoleLogger>::Get()->LogMessage("Hello, {}!", "World");
    */
    class IConsoleLogger
    {
    protected:
        /**
         * @brief Protected implementation of IConsoleLogger::Print.
         * 
         * This function must write the message to internal stream. Message is UTF-8 encoded.
         * 
         * @param message Message to log.
        */
        virtual void PrintImpl(StringSlice message)                     = 0;

        /**
         * @brief Protected implementation of IConsoleLogger::ColoredPrint.
         * 
         * This function must write the message to internal stream using the specified color.
         * Message is UTF-8 encoded.
         * 
         * @param color Text color to use.
         * @param message Message to print.
        */
        virtual void ColoredPrintImpl(Color color, StringSlice message) = 0;

        /**
         * @brief Protected implementation of IConsoleLogger::Log.
         * 
         * The function must print messages in form [MM/DD/YYYY HH:MM:SS] Ferrum3D [message type]: Message
         * 
         * @param type Type of message (information, warning, error).
         * @param message Message to print.
        */
        virtual void LogImpl(LogMessageType type, StringSlice message)  = 0;

    public:
        /**
         * @brief Set levels of debug messages in logger.
         * 
         * This function will set internal logger flags to the specified LogMessageType. This flags
         * will later be used to filter messages from calls to logging functions.
         * 
         * @param types Types of messages to log.
        */
        virtual void SetDebugLevel(LogMessageType types) = 0;

        /**
         * @brief Print a formatted message.
         * 
         * @param fmt Format string.
         * @param args Format arguments.
        */
        template<class... Args>
        FE_FINLINE void Print(StringSlice fmt, Args&&... args)
        {
            PrintImpl(FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        /**
         * @brief Print a formatted message using specified text color.
         * 
         * @param color Text color to use.
         * @param fmt Format string.
         * @param args Format arguments.
        */
        template<class... Args>
        FE_FINLINE void ColoredPrint(Color color, StringSlice fmt, Args&&... args)
        {
            ColoredPrintImpl(color, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        /**
         * @brief Log a message.
         * 
         * This function prints messages in form [MM/DD/YYYY HH:MM:SS] Ferrum3D [message type]: Message
         * 
         * @param type Type of message (information, warning, error)
         * @param fmt Format string.
         * @param args Format arguments.
        */
        template<class... Args>
        inline void Log(LogMessageType type, StringSlice fmt, Args&&... args)
        {
            LogImpl(type, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        /**
         * @brief Log an error.
         *
         * This function prints messages in form [MM/DD/YYYY HH:MM:SS] Ferrum3D [error]: Message
         *
         * @param fmt Format string.
         * @param args Format arguments.
        */
        template<class... Args>
        inline void LogError(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Error, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        /**
         * @brief Log a warning.
         *
         * This function prints messages in form [MM/DD/YYYY HH:MM:SS] Ferrum3D [warning]: Message
         *
         * @param fmt Format string.
         * @param args Format arguments.
        */
        template<class... Args>
        inline void LogWarning(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Warning, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        /**
         * @brief Log a message.
         *
         * This function prints messages in form [MM/DD/YYYY HH:MM:SS] Ferrum3D [message]: Message
         *
         * @param fmt Format string.
         * @param args Format arguments.
        */
        template<class... Args>
        inline void LogMessage(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Message, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }
    };
} // namespace FE::Debug
