#pragma once
#include <FeCore/Time/BaseTime.h>

namespace FE
{
    //! @brief Represents a time span: an interval between two instances of DateTime.
    class TimeSpan final
    {
        TimeValue m_Seconds;

        inline explicit TimeSpan(TimeValue time)
        {
            m_Seconds = time;
        }

    public:
        [[nodiscard]] inline int32_t Days() const
        {
            return static_cast<int32_t>(m_Seconds / (60 * 60 * 24));
        }

        [[nodiscard]] inline int32_t Hours() const
        {
            return static_cast<int32_t>((m_Seconds / (60 * 60)) % 24);
        }

        [[nodiscard]] inline double TotalHours() const
        {
            return m_Seconds / (60.0 * 60.0);
        }

        [[nodiscard]] inline int32_t Minutes() const
        {
            return static_cast<int32_t>((m_Seconds / 60) % 60);
        }

        [[nodiscard]] inline double TotalMinutes() const
        {
            return m_Seconds / 60.0;
        }

        [[nodiscard]] inline int32_t Seconds() const
        {
            return static_cast<int32_t>(m_Seconds % 60);
        }

        [[nodiscard]] inline TimeValue TotalSeconds() const
        {
            return m_Seconds;
        }

        [[nodiscard]] inline bool Empty() const
        {
            return m_Seconds == 0;
        }

        [[nodiscard]] inline TimeSpan operator-() const
        {
            return TimeSpan{ -m_Seconds };
        }

        [[nodiscard]] inline friend TimeSpan operator+(TimeSpan lhs, TimeSpan rhs)
        {
            return TimeSpan{ lhs.m_Seconds + rhs.m_Seconds };
        }

        [[nodiscard]] inline friend TimeSpan operator-(TimeSpan lhs, TimeSpan rhs)
        {
            return TimeSpan{ lhs.m_Seconds - rhs.m_Seconds };
        }

        [[nodiscard]] inline static TimeSpan FromSeconds(TimeValue seconds)
        {
            return TimeSpan(seconds);
        }
    };
} // namespace FE
