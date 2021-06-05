#pragma once
#include <ostream>
#include <string>
#include <sstream>
#include "utf8h/utf8.h"

#undef utf8_nonnull
#undef utf8_null
#undef utf8_pure
#undef utf8_restrict
#undef utf8_weak

#include <iostream>
#include "CoreUtils.h"

namespace Ferrum
{
	/**
	* @brief Convert a UTF-32 character to a c-style string.
	* @param utf32 A valid UTF-32 character.
	* @param buffer pointer to an array with 5 char elements
	*/
	void FE_CORE_API FeUtf32CharToCString(utf8_int32_t utf32, const char* buffer);

	/**
	* @brief Convert a UTF-32 character to a std::string.
	* @param character A valid UTF-32 character.
	* @param str Reference to std::string
	*/
	void FE_CORE_API FeUtf32CharToUtf8String(utf8_int32_t character, std::string& str);

	inline void FeFormatString(std::ostream& stream, const int length, const char* fmt) {
		stream << fmt;
	}

	inline std::string FeFormatString(const std::string& fmt) {
		return fmt;
	}

	/**
	* @brief Format a string and print to an ostream.
	* For example: `FeFormatString(stream, "{} + {} = {}", 2, 2, 4)` will print to stream `"2 + 2 = 4"`.
	* @param stream A reference to std::ostream to print the formatted string to.
	* @param fmt Message format, e.g. `"{} + {} = {}"`.
	* @param args Arguments.
	*/
	template<class T, class ...Args>
	inline void FeFormatString(std::ostream& stream, const int length, const char* fmt, T val, Args... args) {
		const char* iter = fmt;
		const char* start = iter;
		const char* end = start + length + 1;

		utf8_int32_t Code = 0;
		bool escapeNext = false;
		//iter = (const char*)utf8codepoint(iter, &Code);
		for (; iter < end; iter = (const char*)utf8codepoint(iter, &Code)) {
			if (escapeNext) {
				escapeNext = false;
				continue;
			}
			if (Code == '{') {
				iter = (const char*)utf8codepoint(iter, &Code);
				if (Code == '}') {
					stream << val;
					int diff = iter - start;
					FeFormatString(stream, length - diff, iter, args...);
					return;
				}
			}
			else {
				char buffer[5];
				FeUtf32CharToCString(Code, buffer);
				stream << buffer;
			}
		}
	}

	/**
	* @brief Format a string.
	* For example: `FeFormatString("{} + {} = {}", 2, 2, 4)` will return `"2 + 2 = 4"`.
	* @param fmt Message format, e.g. `"{} + {} = {}"`.
	* @param args Arguments.
	* @return A formatted string.
	*/
	template<class T, class ...Args>
	inline std::string FeFormatString(const std::string& fmt, T val, Args... args) {
		std::stringstream ss;
		FeFormatString(ss, fmt.length(), fmt.c_str(), val, args...);
		return std::move(ss.str());
	}
}
