#include <FeCore/Platform/Windows/Common.h>
#include <FeCore/Time/DateTime.h>

namespace FE::Platform
{
    TimeZoneInfo GetTimeZoneInfo()
    {
        TIME_ZONE_INFORMATION info;
        GetTimeZoneInformation(&info);

        TimeZoneInfo result;
        result.MinuteBias = info.Bias;

        if (info.StandardName[0] == 0)
            return result;

        const int32_t length = WideCharToMultiByte(CP_UTF8, 0, info.StandardName, -1, nullptr, 0, nullptr, nullptr);
        result.StandardName.Resize(length, ' ');
        WideCharToMultiByte(
            CP_UTF8, 0, info.StandardName, -1, result.StandardName.Data(), result.StandardName.Size(), nullptr, nullptr);
        return result;
    }


    TimeValue GetCurrentTimeUTC()
    {
        FILETIME fileTime;
        GetSystemTimeAsFileTime(&fileTime);
        return ConvertFiletimeToUnixSeconds(fileTime);
    }


    bool ConvertUTCToLocalTime(SystemTimeInfo source, SystemTimeInfo& result)
    {
        SYSTEMTIME utcTime, localTime;
        ConvertDateTimeToSystemTime(source, utcTime);
        if (!SystemTimeToTzSpecificLocalTime(nullptr, &utcTime, &localTime))
            return false;

        ConvertSystemTimeToDateTime(localTime, result);
        return true;
    }


    bool ConvertLocalTimeToUTC(SystemTimeInfo source, SystemTimeInfo& result)
    {
        SYSTEMTIME localTime, utcTime;
        ConvertDateTimeToSystemTime(source, localTime);
        if (!TzSpecificLocalTimeToSystemTime(nullptr, &localTime, &utcTime))
            return false;

        ConvertSystemTimeToDateTime(utcTime, result);
        return true;
    }
} // namespace FE::Platform
