#pragma once
#include "FeConsole.h"
#include <Time/DateTime.h>
#include <Utils/CoreUtils.h>
#include <Utils/StringUtils.h>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

namespace FE
{
    /**
	* @brief Type of log message
	*/
    enum class LogType
    {
        /**
		* @brief A regular debug message
		*/
        Message,
        /**
		* @brief Warning
		*/
        Warning,
        /**
		* @brief Error
		*/
        Error,
        /**
		* @brief Fatal error
		*/
        FatalError,
        /**
		* @brief Test failure
		*/
        Fail,
        /**
		* @brief Test success
		*/
        Success
    };

    /**
	* @brief Initialize logger
	*/
    inline void InitLogger()
    {
        FeInitConsole();
        FeResetConColor();
    }

    /**
	* @brief Log a formatted message to standard output.
	* For example: `FeLog("{} + {} = {}", 2, 2, 4)` will print 2 + 2 = 4.
	* @param ty LogType of the message.
	* @param fmt Message format, e.g. `"{} + {} = {}"`.
	* @param args Arguments.
	* @return number of characters printed.
	*/
    template<class T, class... Args>
    inline size_t LogTrace(const LogType ty, const std::string_view fmt, T val, Args... args)
    {
        return LogTrace(ty, FeFormatString(fmt, val, args...));
    }

    /**
	* @brief Log a formatted message to standard output.
	* For example: `FeLog("{} + {} = {}", 2, 2, 4)` will print 2 + 2 = 4.
	* @param fmt Message format, e.g. `"{} + {} = {}"`.
	* @param args Arguments.
	* @return number of characters printed.
	*/
    template<class T, class... Args>
    inline size_t LogMsg(const std::string_view fmt, T val, Args... args)
    {
        return LogTrace(LogType::Message, FeFormatString(fmt, val, args...));
    }

    /**
	* @brief Log a message to standard output.
	* @param ty LogType of the message.
	* @param msg Message to print.
	* @return number of characters printed.
	*/
    inline size_t LogTrace(const LogType ty, const std::string_view msg)
    {
#ifndef FE_DEBUG
        if (ty == LogType::Message || ty == LogType::Warning)
            return 0;
#else

#endif
        auto date = DateTime::Now().ToString() + " ";

        std::unique_lock<std::mutex> lk(ConsoleMut);
        size_t res = 10; // size of 'Ferrum3D ['
        FeResetConColor();
        std::cerr << date << FerrumEngineName << " [";
        switch (ty)
        {
        case LogType::Message:
            FeSetConColor(FeConColor::Blue);
            std::cerr << "MESSAGE";
            res += 7;
            break;
        case LogType::Warning:
            FeSetConColor(FeConColor::Yellow);
            std::cerr << "WARNING";
            res += 7;
            break;
        case LogType::Error:
            FeSetConColor(FeConColor::Red);
            std::cerr << "ERROR";
            res += 5;
            break;
        case LogType::FatalError:
            FeSetConColor(FeConColor::Red);
            std::cerr << "FATAL ERROR";
            res += 11;
            break;
        case LogType::Fail:
            FeSetConColor(FeConColor::Red);
            std::cerr << "FAIL";
            res += 4;
            break;
        case LogType::Success:
            FeSetConColor(FeConColor::Green);
            std::cerr << "SUCCESS";
            res += 7;
            break;
        default:
            break;
        }
        FeResetConColor();
        std::cerr << "]: " << msg << "\n"; // 3chars + msg + 1 char
        return res + msg.length() + 4 + date.length();
    }

    /**
	* @brief Log a message to standard output.
	* @param msg Message to print.
	* @return number of characters printed.
	*/
    inline size_t LogMsg(const std::string_view msg)
    {
        return LogTrace(LogType::Message, msg);
    }

#define FE_ASSERT(_Stmt)                                                                                                                   \
    if (!(_Stmt))                                                                                                                          \
    {                                                                                                                                      \
        ::FE::LogTrace(::FE::LogType::Error, "Assertion failed in " __FILE__);                                                             \
        FE_DEBUGBREAK                                                                                                                      \
    }

#define FE_ASSERT_MSG(_Stmt, ...)                                                                                                          \
    if (!(_Stmt))                                                                                                                          \
    {                                                                                                                                      \
        ::FE::LogTrace(::FE::LogType::Error, __VA_ARGS__);                                                                                 \
        FE_DEBUGBREAK                                                                                                                      \
    }
} // namespace FE
