#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Time/BaseTime.h>
#include <festd/vector.h>

namespace FE::Platform
{
    struct WideString
    {
        festd::small_vector<WCHAR, MAX_PATH> m_value;

        WideString(const festd::string_view str)
        {
            const int32_t length = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
            if (length < 0)
                return;

            m_value.resize(length + 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), m_value.data(), m_value.size());
        }

        [[nodiscard]] const WCHAR* data() const
        {
            return m_value.data();
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_value.size();
        }
    };


    template<class TString>
    TString ConvertWideString(const festd::span<const WCHAR> str)
    {
        TString result;

        const int32_t length = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), nullptr, 0, nullptr, nullptr);
        if (length < 0)
            return result;

        result.resize(length, 0);
        WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), result.data(), result.size(), nullptr, nullptr);
        return result;
    }


    template<class TString>
    TString ConvertWideString(const WCHAR* str, size_t length = Constants::kMaxValue<size_t>)
    {
        if (length == Constants::kMaxValue<size_t>)
            length = wcslen(str);

        return ConvertWideString<TString>(festd::span(str, static_cast<uint32_t>(length)));
    }


    inline constexpr int64_t kWindowsTicksPerSecond = 10000000;
    inline constexpr int64_t kWindowsUnixEpochDifference = 11644473600;


    inline TimeValue ConvertWindowsTickToUnixSeconds(const int64_t windowsTicks)
    {
        return windowsTicks / kWindowsTicksPerSecond - kWindowsUnixEpochDifference;
    }


    inline TimeValue ConvertFiletimeToUnixSeconds(const FILETIME fileTime)
    {
        LARGE_INTEGER ftInt;
        ftInt.HighPart = fileTime.dwHighDateTime;
        ftInt.LowPart = fileTime.dwLowDateTime;
        return ConvertWindowsTickToUnixSeconds(static_cast<int64_t>(ftInt.QuadPart));
    }


    inline void ConvertDateTimeToSystemTime(const SystemTimeInfo dateTime, SYSTEMTIME& result)
    {
        result.wYear = static_cast<WORD>(dateTime.Year + 1900);
        result.wMonth = static_cast<WORD>(dateTime.Month) + 1;
        result.wDay = static_cast<WORD>(dateTime.Day);
        result.wDayOfWeek = static_cast<WORD>(dateTime.DayOfWeek);
        result.wHour = static_cast<WORD>(dateTime.Hour);
        result.wMinute = static_cast<WORD>(dateTime.Minute);
        result.wSecond = static_cast<WORD>(dateTime.Second);
        result.wMilliseconds = 0;
    }


    inline void ConvertSystemTimeToDateTime(const SYSTEMTIME& systemTime, SystemTimeInfo& result)
    {
        result.Year = systemTime.wYear - 1900;
        result.Month = systemTime.wMonth;
        result.Day = systemTime.wDay;
        result.DayOfWeek = systemTime.wDayOfWeek;
        result.Hour = systemTime.wHour;
        result.Minute = systemTime.wMinute;
        result.Second = systemTime.wSecond;
    }


    inline IO::FileAttributeFlags ConvertFileAttributeFlags(const DWORD attributes)
    {
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return IO::FileAttributeFlags::kInvalid;

        IO::FileAttributeFlags result = IO::FileAttributeFlags::kNone;
        if (attributes & FILE_ATTRIBUTE_HIDDEN)
            result |= IO::FileAttributeFlags::kHidden;
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
            result |= IO::FileAttributeFlags::kDirectory;
        if (attributes & FILE_ATTRIBUTE_READONLY)
            result |= IO::FileAttributeFlags::kReadOnly;

        return result;
    }


    inline IO::ResultCode ConvertWin32IOError(const DWORD error)
    {
        switch (error)
        {
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return IO::ResultCode::kFileExists;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return IO::ResultCode::kNoFileOrDirectory;
        case ERROR_ACCESS_DENIED:
            return IO::ResultCode::kPermissionDenied;
        case ERROR_SHARING_VIOLATION:
        case ERROR_INVALID_PARAMETER:
            return IO::ResultCode::kInvalidArgument;
        case ERROR_FILE_TOO_LARGE:
            return IO::ResultCode::kFileTooLarge;
        case ERROR_TOO_MANY_OPEN_FILES:
            return IO::ResultCode::kTooManyOpenFiles;
        case ERROR_SEEK:
            return IO::ResultCode::kInvalidSeek;
        case ERROR_NOT_SUPPORTED:
            return IO::ResultCode::kNotSupported;
        default:
            return IO::ResultCode::kUnknownError;
        }
    }
} // namespace FE::Platform
