#pragma once
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/Format.h>
#include <FeCore/Time/TimeSpan.h>

namespace FE
{
    namespace TZ
    {
        struct Convert;
    }


    namespace Internal
    {
        class DateTimeBase
        {
        protected:
            SystemTimeInfo m_Data{};

            DateTimeBase() = default;

            DateTimeBase(SystemTimeInfo data)
                : m_Data(data)
            {
            }

        public:
            [[nodiscard]] int32_t Year() const
            {
                return static_cast<int32_t>(m_Data.Year) + 1900;
            }

            [[nodiscard]] int32_t Month() const
            {
                return m_Data.Month;
            }

            [[nodiscard]] int32_t Day() const
            {
                return m_Data.Day;
            }

            [[nodiscard]] int32_t DayOfWeek() const
            {
                return m_Data.DayOfWeek;
            }

            [[nodiscard]] int32_t Hour() const
            {
                return m_Data.Hour;
            }

            [[nodiscard]] int32_t Minute() const
            {
                return m_Data.Minute;
            }

            [[nodiscard]] int32_t Second() const
            {
                return m_Data.Second;
            }

            [[nodiscard]] FixStr64 ToString(DateTimeFormatKind formatKind) const;
        };
    } // namespace Internal


    //! @brief A struct that represents date and time, template parameter specifies the time zone.
    //!
    //! \see TZ::UTC, TZ::Local
    template<class TTimeZone>
    class DateTime final : public Internal::DateTimeBase
    {
        friend TZ::Convert;

        DateTime(SystemTimeInfo data)
            : Internal::DateTimeBase(data)
        {
        }

    public:
        DateTime() = default;

        [[nodiscard]] static DateTime Now()
        {
            return FromUnixTime(Platform::GetCurrentTimeUTC());
        }

        [[nodiscard]] TimeValue ToUnixTime() const
        {
            return TTimeZone::ToUnixTime(m_Data);
        }

        [[nodiscard]] static DateTime FromUnixTime(TimeValue time)
        {
            return DateTime{ TTimeZone::FromUnixTime(time) };
        }
    };


    template<class TTimeZone1, class TTimeZone2>
    [[nodiscard]] TimeSpan operator-(DateTime<TTimeZone1> lhs, DateTime<TTimeZone2> rhs)
    {
        if constexpr (std::is_same_v<TTimeZone1, TTimeZone2>)
        {
            // Avoid conversion when we know the time zones are the same.
            const auto a = festd::bit_cast<SystemTimeInfo>(lhs);
            const auto b = festd::bit_cast<SystemTimeInfo>(rhs);
            return TimeSpan::FromSeconds(Platform::ConstructTime(a) - Platform::ConstructTime(b));
        }

        return TimeSpan::FromSeconds(lhs.ToUnixTime() - rhs.ToUnixTime());
    }


    template<class TTimeZone1, class TTimeZone2>
    [[nodiscard]] bool operator==(DateTime<TTimeZone1> lhs, DateTime<TTimeZone2> rhs)
    {
        if constexpr (std::is_same_v<TTimeZone1, TTimeZone2>)
        {
            return festd::bit_cast<uint64_t>(lhs) == festd::bit_cast<uint64_t>(rhs);
        }

        return lhs.ToUnixTime() == rhs.ToUnixTime();
    }


    template<class TTimeZone1, class TTimeZone2>
    [[nodiscard]] bool operator!=(DateTime<TTimeZone1> lhs, DateTime<TTimeZone2> rhs)
    {
        return !(lhs == rhs);
    }


    namespace TZ
    {
        struct UTC final
        {
            UTC() = delete;

            [[nodiscard]] static TimeValue ToUnixTime(SystemTimeInfo dateTime)
            {
                return Platform::ConstructTime(dateTime);
            }

            [[nodiscard]] static SystemTimeInfo FromUnixTime(TimeValue time)
            {
                return Platform::DeconstructTime(time);
            }
        };


        struct Local final
        {
            Local() = delete;

            [[nodiscard]] static TimeValue ToUnixTime(SystemTimeInfo dateTime)
            {
                SystemTimeInfo utc;
                Platform::ConvertLocalTimeToUTC(dateTime, utc);
                return Platform::ConstructTime(utc);
            }

            [[nodiscard]] static SystemTimeInfo FromUnixTime(TimeValue time)
            {
                SystemTimeInfo local;
                Platform::ConvertUTCToLocalTime(Platform::DeconstructTime(time), local);
                return local;
            }
        };


        struct Convert final
        {
            Convert() = delete;

            template<class TDestTimeZone, class TSourceTimeZone>
            [[nodiscard]] static DateTime<TDestTimeZone> To(DateTime<TSourceTimeZone> dateTime)
            {
                return DateTime<TDestTimeZone>::FromUnixTime(dateTime.ToUnixTime());
            }
        };
    } // namespace TZ
} // namespace FE
