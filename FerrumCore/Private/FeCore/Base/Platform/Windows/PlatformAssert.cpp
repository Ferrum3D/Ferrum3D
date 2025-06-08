#include <FeCore/Base/Base.h>
#include <FeCore/Base/PlatformInclude.h>

namespace FE::Platform
{
    void AssertionReport(const SourceLocation sourceLocation, const char* message, const uint32_t messageSize, const bool crash)
    {
        //
        // NOTE: We cannot use WideString here: it can allocate memory if its inline buffer overflows.
        // But the Platform::AssertionReport function might have been called from the memory management code,
        // which means that an allocation here can lead to infinite recursion or memory corruption.
        //

        constexpr int32_t kMaxMessageSize = 8 * 1024;

        const wchar_t* wideMessage = L"Unknown Error (Error message had invalid encoding)";
        [[maybe_unused]] const wchar_t* wideFilename = L"Unknown File (Filename had invalid encoding)";

        const int32_t clampedMessageSize = Math::Min(static_cast<int32_t>(messageSize), kMaxMessageSize);
        const int32_t messageLength = MultiByteToWideChar(CP_UTF8, 0, message, clampedMessageSize, nullptr, 0);
        const int32_t filenameLength = MultiByteToWideChar(CP_UTF8, 0, sourceLocation.m_fileName, -1, nullptr, 0);

        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);

        const size_t requiredBufferSize = filenameLength + messageLength + 4 + std::numeric_limits<uint32_t>::digits10;
        const size_t allocationSize = AlignUp(requiredBufferSize, systemInfo.dwAllocationGranularity);
        void* allocation = VirtualAlloc(nullptr, allocationSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        wchar_t* buffer = static_cast<wchar_t*>(allocation);

        if (filenameLength > 0)
        {
            const int32_t limitedSize = Math::Min(filenameLength, MAX_PATH + 1);
            MultiByteToWideChar(CP_UTF8, 0, sourceLocation.m_fileName, -1, buffer, limitedSize - 1);
            wideFilename = buffer;
            buffer += limitedSize;

            if (crash)
            {
                // Split filename and message for _wassert
                *buffer++ = L'\0';
            }
            else
            {
                buffer += _swprintf(buffer, L"(%d): ", sourceLocation.m_lineNumber);
            }
        }

        if (messageLength > 0)
        {
            const int32_t limitedSize = Math::Min(messageLength, kMaxMessageSize);
            MultiByteToWideChar(CP_UTF8, 0, message, clampedMessageSize, buffer, limitedSize - 1);
            buffer[limitedSize] = L'\0';
            wideMessage = buffer;
        }

        OutputDebugStringW(wideMessage);

        if (crash)
        {
#if FE_DEBUG
            _wassert(wideMessage, wideFilename, sourceLocation.m_lineNumber);
#endif
            FE_DebugBreak();
        }

        VirtualFree(allocation, 0, MEM_RELEASE);
    }
} // namespace FE::Platform
