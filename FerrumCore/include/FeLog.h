#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include "StringUtils.h"
#include "CoreUtils.h"
#include "FeConsole.h"

namespace Ferrum
{
	/**
	* @brief Type of log message
	*/
	enum class FeLogType
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
	inline void FeLogInit() {
		FeInitConsole();
		FeResetConColor();
	}

	/**
	* @brief Log a formatted message to standard output.
	* For example: `FeLog("{} + {} = {}", 2, 2, 4)` will print 2 + 2 = 4.
	* @param ty FeLogType of the message.
	* @param fmt Message format, e.g. `"{} + {} = {}"`.
	* @param args Arguments.
	* @return number of characters printed.
	*/
	template<class T, class ...Args>
	inline size_t FeLog(const FeLogType ty, const std::string& fmt, T val, Args... args) {
		return FeLog(ty, FeFormatString(fmt, val, args...));
	}

	/**
	* @brief Log a message to standard output.
	* @param ty FeLogType of the message.
	* @param msg Message to print.
	* @return number of characters printed.
	*/
	inline size_t FeLog(const FeLogType ty, const std::string& msg) {
#ifndef FE_DEBUG
		if (ty == FeLogType::Message || ty == FeLogType::Warning) {
			return 0;
		}
#else

#endif
		std::unique_lock<std::mutex> lk(ConsoleMut);
		size_t res = 10; // size of 'Ferrum3D ['
		FeResetConColor();
		std::cerr << FerrumEngineName << " [";
		switch (ty)
		{
		case FeLogType::Message:
			FeSetConColor(FeConColor::Blue);
			std::cerr << "MESSAGE"; res += 7;
			break;
		case FeLogType::Warning:
			FeSetConColor(FeConColor::Yellow);
			std::cerr << "WARNING"; res += 7;
			break;
		case FeLogType::Error:
			FeSetConColor(FeConColor::Red);
			std::cerr << "ERROR"; res += 5;
			break;
		case FeLogType::FatalError:
			FeSetConColor(FeConColor::Red);
			std::cerr << "FATAL ERROR"; res += 11;
			break;
		case FeLogType::Fail:
			FeSetConColor(FeConColor::Red);
			std::cerr << "FAIL"; res += 4;
			break;
		case FeLogType::Success:
			FeSetConColor(FeConColor::Green);
			std::cerr << "SUCCESS"; res += 7;
			break;
		default:
			break;
		}
		FeResetConColor();
		std::cerr << "]: " << msg << "\n"; // 3chars + msg + 1 char
		return res + msg.length() + 4;
	}

#define FE_ASSERT(_Stmt) if (!(_Stmt)) { ::Ferrum::FeLog(::Ferrum::FeLogType::Error, "Assertion failed in " __FILE__); FE_DEBUGBREAK }

#define FE_ASSERT_MSG(_Stmt, ...) if (!(_Stmt)) { ::Ferrum::FeLog(::Ferrum::FeLogType::Error, __VA_ARGS__); FE_DEBUGBREAK }
}
