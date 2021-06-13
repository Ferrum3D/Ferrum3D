#include "StringUtils.h"

namespace Ferrum
{
	// Encodes the UTF-32 encoded char into a UTF-8 string. 
	// Stores the result in the buffer and returns the position 
	// of the end of the buffer
	// (unchecked access, be sure to provide a buffer that is big enough)
	inline static char* char_utf32_to_utf8(utf8_int32_t utf32, const char* buffer)
	{
		char* end = const_cast<char*>(buffer);
		if (utf32 < 0x7F) *(end++) = static_cast<unsigned>(utf32);
		else if (utf32 < 0x7FF) {
			*(end++) = 0b1100'0000 + static_cast<unsigned>(utf32 >> 6);
			*(end++) = 0b1000'0000 + static_cast<unsigned>(utf32 & 0b0011'1111);
		}
		else if (utf32 < 0x10000) {
			*(end++) = 0b1110'0000 + static_cast<unsigned>(utf32 >> 12);
			*(end++) = 0b1000'0000 + static_cast<unsigned>((utf32 >> 6) & 0b0011'1111);
			*(end++) = 0b1000'0000 + static_cast<unsigned>(utf32 & 0b0011'1111);
		}
		else if (utf32 < 0x110000) {
			*(end++) = 0b1111'0000 + static_cast<unsigned>(utf32 >> 18);
			*(end++) = 0b1000'0000 + static_cast<unsigned>((utf32 >> 12) & 0b0011'1111);
			*(end++) = 0b1000'0000 + static_cast<unsigned>((utf32 >> 6) & 0b0011'1111);
			*(end++) = 0b1000'0000 + static_cast<unsigned>(utf32 & 0b0011'1111);
		}
		*end = '\0';
		return end;
	}

	void FeUtf32CharToCString(utf8_int32_t utf32, const char* buffer)
	{
		char* end = const_cast<char*>(buffer);
		if (utf32 < 0x7F) *(end++) = static_cast<unsigned>(utf32);
		else if (utf32 < 0x7FF) {
			*(end++) = 0b1100'0000 + static_cast<unsigned>(utf32 >> 6);
			*(end++) = 0b1000'0000 + static_cast<unsigned>(utf32 & 0b0011'1111);
		}
		else if (utf32 < 0x10000) {
			*(end++) = 0b1110'0000 + static_cast<unsigned>(utf32 >> 12);
			*(end++) = 0b1000'0000 + static_cast<unsigned>((utf32 >> 6) & 0b0011'1111);
			*(end++) = 0b1000'0000 + static_cast<unsigned>(utf32 & 0b0011'1111);
		}
		else if (utf32 < 0x110000) {
			*(end++) = 0b1111'0000 + static_cast<unsigned>(utf32 >> 18);
			*(end++) = 0b1000'0000 + static_cast<unsigned>((utf32 >> 12) & 0b0011'1111);
			*(end++) = 0b1000'0000 + static_cast<unsigned>((utf32 >> 6) & 0b0011'1111);
			*(end++) = 0b1000'0000 + static_cast<unsigned>(utf32 & 0b0011'1111);
		}
		*end = '\0';
	}

	void FeUtf32CharToUtf8String(utf8_int32_t character, std::string& str) {
		const char buf[5]{};
		FeUtf32CharToCString(character, buf);
		str = std::move(std::string(buf));
	}
}
