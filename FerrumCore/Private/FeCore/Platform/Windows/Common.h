#pragma once
#include <FeCore/Base/PlatformInclude.h>
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
        result.wYear = dateTime.Year + 1900;
        result.wMonth = dateTime.Month;
        result.wDay = dateTime.Day;
        result.wDayOfWeek = dateTime.DayOfWeek;
        result.wHour = dateTime.Hour;
        result.wMinute = dateTime.Minute;
        result.wSecond = dateTime.Second;
        result.wMilliseconds = 0;
    }


    inline void ConvertSystemTimeToDateTime(const SYSTEMTIME& systemTime, SystemTimeInfo& result)
    {
        result.Year = systemTime.wYear - 1900;
        result.Month = static_cast<int8_t>(systemTime.wMonth);
        result.Day = static_cast<int8_t>(systemTime.wDay);
        result.DayOfWeek = static_cast<int8_t>(systemTime.wDayOfWeek);
        result.Hour = static_cast<int8_t>(systemTime.wHour);
        result.Minute = static_cast<int8_t>(systemTime.wMinute);
        result.Second = static_cast<int8_t>(systemTime.wSecond);
    }
} // namespace FE::Platform
