#include <Core/Strings/Encoding.h>

namespace FE::Str
{
    uint32_t ConvertUtf8ToUtf16(const char* source, const uint32_t sourceSize, char16_t* destination,
                                const uint32_t destinationSize)
    {
        if (destinationSize == 0)
            return 0;

        const bool nullTerminated = sourceSize == Constants::kMaxU32;

        uint32_t written = 0;
        for (const char* iter = source;;)
        {
            int32_t remainingSize;
            if (nullTerminated)
            {
                if (*iter == 0)
                    break;

                remainingSize = -1;
            }
            else
            {
                if (iter >= source + sourceSize)
                    break;

                remainingSize = static_cast<int32_t>(sourceSize - (iter - source));
            }

            if (const char firstByte = *iter; static_cast<uint8_t>(firstByte) < 0x80)
            {
                if (firstByte == 0)
                    break;

                if (written + 1 >= destinationSize)
                    break;

                destination[written++] = static_cast<char16_t>(firstByte);
                ++iter;
                continue;
            }

            int32_t codepoint;
            const int32_t read = UTF8::DecodeForward(iter, remainingSize, &codepoint);
            if (read < 0)
                return Constants::kMaxU32;

            char16_t temp[2];
            const int32_t needToWrite = UTF16::Encode(codepoint, temp);
            if (needToWrite < 0)
                return Constants::kMaxU32;

            if (written + needToWrite + 1 > destinationSize)
                break;

            memcpy(&destination[written], temp, sizeof(char16_t) * needToWrite);
            written += needToWrite;
            iter += read;
        }

        destination[written++] = 0;
        return written;
    }


    uint32_t ConvertUtf16ToUtf8(const char16_t* source, const uint32_t sourceSize, char* destination,
                                const uint32_t destinationSize)
    {
        if (destinationSize == 0)
            return 0;

        const bool nullTerminated = sourceSize == Constants::kMaxU32;

        uint32_t written = 0;
        for (const char16_t* iter = source;;)
        {
            int32_t remainingSize;
            if (nullTerminated)
            {
                if (*iter == 0)
                    break;

                remainingSize = -1;
            }
            else
            {
                if (iter >= source + sourceSize)
                    break;

                remainingSize = static_cast<int32_t>(sourceSize - (iter - source));
            }

            int32_t codepoint;
            const int32_t read = UTF16::DecodeForward(iter, remainingSize, &codepoint);
            if (read < 0)
                return Constants::kMaxU32;

            char temp[4];
            const int32_t needToWrite = UTF8::Encode(codepoint, temp);
            if (needToWrite <= 0)
                return Constants::kMaxU32;

            if (written + needToWrite + 1 > destinationSize)
                break;

            memcpy(&destination[written], temp, sizeof(char) * needToWrite);
            written += needToWrite;
            iter += read;
        }

        destination[written++] = 0;
        return written;
    }
} // namespace FE::Str
