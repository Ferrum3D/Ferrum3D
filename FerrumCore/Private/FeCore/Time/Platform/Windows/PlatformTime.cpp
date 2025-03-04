#include <FeCore/Platform/Windows/Common.h>
#include <FeCore/Time/DateTime.h>

namespace FE::Platform
{
    TimeZoneInfo GetTimeZoneInfo()
    {
        TIME_ZONE_INFORMATION info;
        GetTimeZoneInformation(&info);

        TimeZoneInfo result;
        result.m_minuteBias = info.Bias;

        if (info.StandardName[0] == 0)
            return result;

        result.m_standardName = Env::Name{ ConvertWideString<festd::fixed_string>(info.StandardName) };
        return result;
    }


    TimeValue GetCurrentTimeUTC()
    {
        FILETIME fileTime;
        GetSystemTimeAsFileTime(&fileTime);
        return ConvertFiletimeToUnixSeconds(fileTime);
    }


    bool ConvertUTCToLocalTime(const SystemTimeInfo source, SystemTimeInfo& result)
    {
        SYSTEMTIME utcTime, localTime;
        ConvertDateTimeToSystemTime(source, utcTime);
        if (!SystemTimeToTzSpecificLocalTime(nullptr, &utcTime, &localTime))
            return false;

        ConvertSystemTimeToDateTime(localTime, result);
        return true;
    }


    bool ConvertLocalTimeToUTC(const SystemTimeInfo source, SystemTimeInfo& result)
    {
        SYSTEMTIME localTime, utcTime;
        ConvertDateTimeToSystemTime(source, localTime);
        if (!TzSpecificLocalTimeToSystemTime(nullptr, &localTime, &utcTime))
            return false;

        ConvertSystemTimeToDateTime(utcTime, result);
        return true;
    }
} // namespace FE::Platform
