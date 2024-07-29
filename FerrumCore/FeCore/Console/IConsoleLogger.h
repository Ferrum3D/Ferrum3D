#pragma once
#include <FeCore/Console/Console.h>
#include <FeCore/Strings/Format.h>

namespace FE::Debug
{
    using FE::Console::Color;

    //! \brief Type of log message.
    enum class LogMessageType
    {
        //! \brief No flags.
        None = 0,

        //! \brief A regular debug message.
        Message = 1 << 0,

        //! \brief Warning.
        Warning = 1 << 1,

        //! \brief Error.
        Error = 1 << 2,

        //! \brief All messages
        All = Message | Warning | Error
    };

    FE_ENUM_OPERATORS(LogMessageType);

    //! \brief Interface that console loggers must implement.
    //!
    //! All implementations must inherit from \ref FE::SharedInterfaceImplBase<IConsoleLogger>, not the interface directly.
    //! The loggers work with UTF-8 encoding and must set up their internal stream to use unicode.<br>
    //! <br>
    //! Usage:
    //! \code{.cpp}
    //!     FE::ServiceLocator<FE::Debug::IConsoleLogger>::Get()->LogMessage("Hello, {}!", "World");
    //! \endcode
    class IConsoleLogger : public Memory::RefCountedObjectBase
    {
    protected:
        //! \brief Protected implementation of \ref IConsoleLogger::Print.
        //!
        //! This function must write the message to internal stream. Message is UTF-8 encoded.
        //!
        //! \param [in] message - Message to log.
        virtual void PrintImpl(StringSlice message) = 0;

        //! \brief Protected implementation of \ref IConsoleLogger::ColoredPrint.
        //!
        //! This function must write the message to internal stream using the specified color.
        //! Message is UTF-8 encoded.
        //!
        //! \param [in] color   - Text color to use.
        //! \param [in] message - Message to print.
        virtual void ColoredPrintImpl(Color color, StringSlice message) = 0;

        //! \brief Protected implementation of \ref IConsoleLogger::Log.
        //!
        //! The function must print messages in form `[MM/DD/YYYY HH:MM:SS] Ferrum3D [message type]: Message`
        //!
        //! \param [in] type    - Type of message (information, warning, error).
        //! \param [in] message - Message to print.
        virtual void LogImpl(LogMessageType type, StringSlice message) = 0;

    public:
        FE_CLASS_RTTI(IConsoleLogger, "0A75661C-E88C-4263-9095-785EB3CEECB8");

        //! \brief Set levels of debug messages in logger.
        //!
        //! This function will set internal logger flags to the specified \ref LogMessageType. This flags
        //! will later be used to filter messages from calls to logging functions.
        //!
        //! \param [in] types - Types of messages to log.
        virtual void SetDebugLevel(LogMessageType types) = 0;

        //! \brief Print a formatted message.
        //!
        //! \param [in] fmt  - Format string.
        //! \param [in] args - Format arguments.
        template<class... Args>
        FE_FORCE_INLINE void Print(StringSlice fmt, Args&&... args)
        {
            PrintImpl(FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        //! \brief Print a formatted message using specified text color.
        //!
        //! \param [in] color - Text color to use.
        //! \param [in] fmt   - Format string.
        //! \param [in] args  - Format arguments.
        template<class... Args>
        FE_FORCE_INLINE void ColoredPrint(Color color, StringSlice fmt, Args&&... args)
        {
            ColoredPrintImpl(color, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        //! \brief Log a message.
        //!
        //! This function prints messages in form `[MM/DD/YYYY HH:MM:SS] Ferrum3D [message type]: Message`.
        //!
        //! \param [in] type - Type of message (information, warning, error)
        //! \param [in] fmt  - Format string.
        //! \param [in] args - Format arguments.
        template<class... Args>
        inline void Log(LogMessageType type, StringSlice fmt, Args&&... args)
        {
            LogImpl(type, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        //! \brief Log an error.
        //!
        //! This function prints messages in form `[MM/DD/YYYY HH:MM:SS] Ferrum3D [error]: Message`.
        //!
        //! \param [in] fmt  - Format string.
        //! \param [in] args - Format arguments.
        template<class... Args>
        inline void LogError(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Error, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        //! \brief Log a warning.
        //!
        //! This function prints messages in form `[MM/DD/YYYY HH:MM:SS] Ferrum3D [warning]: Message`.
        //!
        //! \param [in] fmt  - Format string.
        //! \param [in] args - Format arguments.
        template<class... Args>
        inline void LogWarning(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Warning, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }

        //! \brief Log a message.
        //!
        //! This function prints messages in form `[MM/DD/YYYY HH:MM:SS] Ferrum3D [message]: Message`.
        //!
        //! \param [in] fmt  - Format string.
        //! \param [in] args - Format arguments.
        template<class... Args>
        inline void LogMessage(StringSlice fmt, Args&&... args)
        {
            LogImpl(LogMessageType::Message, FE::Fmt::Format(fmt, std::forward<Args>(args)...));
        }
    };
} // namespace FE::Debug
