#pragma once
#include <FeCore/Time/BaseTime.h>

namespace FE
{
    //! @brief Represents a time span: an interval between two instances of DateTime.
    struct TimeSpan final
    {
        [[nodiscard]] int32_t Days() const
        {
            return static_cast<int32_t>(m_seconds / (UINT64_C(60 * 60) * 24));
        }

        [[nodiscard]] int32_t Hours() const
        {
            return static_cast<int32_t>((m_seconds / UINT64_C(60 * 60)) % 24);
        }

        [[nodiscard]] double TotalHours() const
        {
            return static_cast<double>(m_seconds) / (60.0 * 60.0);
        }

        [[nodiscard]] int32_t Minutes() const
        {
            return static_cast<int32_t>((m_seconds / 60) % 60);
        }

        [[nodiscard]] double TotalMinutes() const
        {
            return static_cast<double>(m_seconds) / 60.0;
        }

        [[nodiscard]] int32_t Seconds() const
        {
            return static_cast<int32_t>(m_seconds % 60);
        }

        [[nodiscard]] TimeValue TotalSeconds() const
        {
            return m_seconds;
        }

        [[nodiscard]] bool Empty() const
        {
            return m_seconds == 0;
        }

        [[nodiscard]] TimeSpan operator-() const
        {
            return TimeSpan{ -m_seconds };
        }

        [[nodiscard]] friend TimeSpan operator+(const TimeSpan lhs, const TimeSpan rhs)
        {
            return TimeSpan{ lhs.m_seconds + rhs.m_seconds };
        }

        [[nodiscard]] friend TimeSpan operator-(const TimeSpan lhs, const TimeSpan rhs)
        {
            return TimeSpan{ lhs.m_seconds - rhs.m_seconds };
        }

        [[nodiscard]] static TimeSpan FromSeconds(const TimeValue seconds)
        {
            return TimeSpan(seconds);
        }

    private:
        TimeValue m_seconds;

        explicit TimeSpan(const TimeValue time)
        {
            m_seconds = time;
        }
    };
} // namespace FE
