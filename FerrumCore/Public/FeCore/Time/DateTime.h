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

            inline DateTimeBase() = default;

            inline DateTimeBase(SystemTimeInfo data)
                : m_Data(data)
            {
            }

        public:
            [[nodiscard]] inline int32_t Year() const
            {
                return static_cast<int32_t>(m_Data.Year) + 1900;
            }

            [[nodiscard]] inline int32_t Month() const
            {
                return m_Data.Month;
            }

            [[nodiscard]] inline int32_t Day() const
            {
                return m_Data.Day;
            }

            [[nodiscard]] inline int32_t DayOfWeek() const
            {
                return m_Data.DayOfWeek;
            }

            [[nodiscard]] inline int32_t Hour() const
            {
                return m_Data.Hour;
            }

            [[nodiscard]] inline int32_t Minute() const
            {
                return m_Data.Minute;
            }

            [[nodiscard]] inline int32_t Second() const
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

        inline DateTime(SystemTimeInfo data)
            : Internal::DateTimeBase(data)
        {
        }

    public:
        inline DateTime() = default;

        [[nodiscard]] inline static DateTime Now()
        {
            return FromUnixTime(Platform::GetCurrentTimeUTC());
        }

        [[nodiscard]] inline TimeValue ToUnixTime() const
        {
            return TTimeZone::ToUnixTime(m_Data);
        }

        [[nodiscard]] inline static DateTime FromUnixTime(TimeValue time)
        {
            return DateTime{ TTimeZone::FromUnixTime(time) };
        }
    };


    template<class TTimeZone1, class TTimeZone2>
    [[nodiscard]] inline TimeSpan operator-(DateTime<TTimeZone1> lhs, DateTime<TTimeZone2> rhs)
    {
        if constexpr (std::is_same_v<TTimeZone1, TTimeZone2>)
        {
            // Avoid conversion when we know the time zones are the same.
            const auto a = bit_cast<SystemTimeInfo>(lhs);
            const auto b = bit_cast<SystemTimeInfo>(rhs);
            return TimeSpan::FromSeconds(Platform::ConstructTime(a) - Platform::ConstructTime(b));
        }

        return TimeSpan::FromSeconds(lhs.ToUnixTime() - rhs.ToUnixTime());
    }


    template<class TTimeZone1, class TTimeZone2>
    [[nodiscard]] inline bool operator==(DateTime<TTimeZone1> lhs, DateTime<TTimeZone2> rhs)
    {
        if constexpr (std::is_same_v<TTimeZone1, TTimeZone2>)
        {
            return bit_cast<uint64_t>(lhs) == bit_cast<uint64_t>(rhs);
        }

        return lhs.ToUnixTime() == rhs.ToUnixTime();
    }


    template<class TTimeZone1, class TTimeZone2>
    [[nodiscard]] inline bool operator!=(DateTime<TTimeZone1> lhs, DateTime<TTimeZone2> rhs)
    {
        return !(lhs == rhs);
    }


    namespace TZ
    {
        struct UTC final
        {
            UTC() = delete;

            [[nodiscard]] inline static TimeValue ToUnixTime(SystemTimeInfo dateTime)
            {
                return Platform::ConstructTime(dateTime);
            }

            [[nodiscard]] inline static SystemTimeInfo FromUnixTime(TimeValue time)
            {
                return Platform::DeconstructTime(time);
            }
        };


        struct Local final
        {
            Local() = delete;

            [[nodiscard]] inline static TimeValue ToUnixTime(SystemTimeInfo dateTime)
            {
                SystemTimeInfo utc;
                Platform::ConvertLocalTimeToUTC(dateTime, utc);
                return Platform::ConstructTime(utc);
            }

            [[nodiscard]] inline static SystemTimeInfo FromUnixTime(TimeValue time)
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
            [[nodiscard]] inline static DateTime<TDestTimeZone> To(DateTime<TSourceTimeZone> dateTime)
            {
                return DateTime<TDestTimeZone>::FromUnixTime(dateTime.ToUnixTime());
            }
        };
    } // namespace TZ
} // namespace FE
