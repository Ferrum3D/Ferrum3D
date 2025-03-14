#include <FeCore/Strings/Encoding.h>

namespace FE::Str
{
    uint32_t ConvertUtf8ToUtf16(const char* source, const uint32_t sourceSize, char16_t* destination,
                                const uint32_t destinationSize)
    {
        uint32_t charsWritten = 0;
        for (const char* iter = source; iter < source + sourceSize;)
        {
            if (const char firstByte = *iter; static_cast<uint8_t>(firstByte) < 0x80)
            {
                if (firstByte == 0)
                    break;

                if (charsWritten >= destinationSize)
                    return Constants::kMaxU32;

                destination[charsWritten++] = static_cast<char16_t>(firstByte);
                ++iter;
                continue;
            }

            int32_t codepoint;
            const int32_t remainingSize = static_cast<int32_t>(sourceSize - (iter - source));
            const int32_t bytesRead = UTF8::DecodeForward(iter, remainingSize, &codepoint);
            if (bytesRead < 0)
                return Constants::kMaxU32;

            charsWritten += UTF16::Encode(codepoint, &destination[charsWritten]);
            iter += bytesRead;

            FE_CoreAssertDebug(charsWritten < destinationSize);
        }

        destination[charsWritten++] = 0;
        return charsWritten;
    }


    uint32_t ConvertUtf16ToUtf8(const char16_t* source, const uint32_t sourceSize, char* destination,
                                const uint32_t destinationSize)
    {
        uint32_t bytesWritten = 0;
        for (const char16_t* iter = source; iter < source + sourceSize;)
        {
            if (*iter == 0)
                break;

            if (bytesWritten >= destinationSize)
                return Constants::kMaxU32;

            int32_t codepoint;
            const int32_t remainingSize = static_cast<int32_t>(sourceSize - (iter - source));
            const int32_t charsRead = UTF16::DecodeForward(iter, remainingSize, &codepoint);
            if (charsRead < 0)
                return Constants::kMaxU32;

            bytesWritten += UTF8::Encode(codepoint, &destination[bytesWritten]);
            iter += charsRead;

            FE_CoreAssertDebug(bytesWritten < destinationSize);
        }

        destination[bytesWritten++] = 0;
        return bytesWritten;
    }
} // namespace FE::Str
