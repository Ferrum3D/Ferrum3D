#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Containers/SmallVector.h>
#include <FeCore/Time/BaseTime.h>

namespace FE::Platform
{
    template<uint32_t TLength>
    struct WideString
    {
        festd::small_vector<WCHAR, TLength> Value;

        inline WideString(StringSlice str)
        {
            const int32_t length = MultiByteToWideChar(CP_UTF8, 0, str.Data(), str.Size(), nullptr, 0);
            if (length < 0)
                return;

            Value.resize(length + 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.Data(), str.Size(), Value.data(), Value.size());
        }
    };


    inline constexpr int64_t WindowsTicksPerSecond = 10000000;
    inline constexpr int64_t WindowsUnixEpochDifference = 11644473600;


    inline static TimeValue ConvertWindowsTickToUnixSeconds(int64_t windowsTicks)
    {
        return windowsTicks / WindowsTicksPerSecond - WindowsUnixEpochDifference;
    }


    inline static TimeValue ConvertFiletimeToUnixSeconds(FILETIME fileTime)
    {
        LARGE_INTEGER ftInt;
        ftInt.HighPart = fileTime.dwHighDateTime;
        ftInt.LowPart = fileTime.dwLowDateTime;
        return ConvertWindowsTickToUnixSeconds(static_cast<int64_t>(ftInt.QuadPart));
    }


    inline static void ConvertDateTimeToSystemTime(SystemTimeInfo dateTime, SYSTEMTIME& result)
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


    inline static void ConvertSystemTimeToDateTime(const SYSTEMTIME& systemTime, SystemTimeInfo& result)
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
