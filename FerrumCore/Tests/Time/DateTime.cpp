#include <FeCore/Time/DateTime.h>
#include <Tests/Common/TestCommon.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(DateTime, Basic)
{
    const auto time = DateTime<TZ::UTC>::FromUnixTime(1724774551);

    EXPECT_EQ(time.Year(), 2024);
    EXPECT_EQ(time.Month(), 7);
    EXPECT_EQ(time.Day(), 27);
    EXPECT_EQ(time.Hour(), 16);
    EXPECT_EQ(time.Minute(), 2);
    EXPECT_EQ(time.Second(), 31);
    EXPECT_EQ(time.DayOfWeek(), 2);
}

TEST(DateTime, ConvertTimeZone)
{
    const auto timeZoneInfo = Platform::GetTimeZoneInfo();
    printf("Time zone: \"%s\" (%d bias)\n", timeZoneInfo.m_standardName.Data(), timeZoneInfo.m_minuteBias);

    const DateTime<TZ::Local> localTime = DateTime<TZ::Local>::Now();
    const DateTime<TZ::UTC> utcTime = TZ::Convert::To<TZ::UTC>(localTime);

    const TimeSpan diff = utcTime - localTime;
    EXPECT_EQ(diff.TotalSeconds(), 0);
    EXPECT_TRUE(diff.Empty());

    const auto normalizeMinuteBias = [](int32_t minuteBias) {
        while (minuteBias < 0)
            minuteBias += 24 * 60;
        return minuteBias;
    };

    const uint32_t minuteDiff = utcTime.Minute() - localTime.Minute();
    const uint32_t hourDiff = utcTime.Hour() - localTime.Hour();
    EXPECT_EQ(normalizeMinuteBias(minuteDiff + hourDiff * 60), normalizeMinuteBias(timeZoneInfo.m_minuteBias));

    EXPECT_EQ(localTime, TZ::Convert::To<TZ::Local>(utcTime));
    EXPECT_EQ(utcTime, TZ::Convert::To<TZ::UTC>(localTime));
    EXPECT_EQ(utcTime, TZ::Convert::To<TZ::UTC>(utcTime));
    EXPECT_EQ(localTime, TZ::Convert::To<TZ::Local>(localTime));
}
