#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Base/Platform.h>
#include <ctime>
#include <ostream>
#include <sstream>

namespace FE
{
    //! \brief Represents a time span: an interval between two instances of \ref DateTime.
    class TimeSpan
    {
        tm m_Data;

        inline TimeSpan(time_t time)
        {
            m_Data = *localtime(&time);
        }

    public:
        FE_RTTI_Class(TimeSpan, "F42DAA6C-53F3-4AA5-9971-9783D8754F6C");

        inline int32_t Years() const
        {
            return m_Data.tm_year;
        }

        inline int32_t Days() const
        {
            return m_Data.tm_mday;
        }

        inline int32_t Hours() const
        {
            return m_Data.tm_hour;
        }

        inline int32_t Minutes() const
        {
            return m_Data.tm_min;
        }

        inline int32_t Seconds() const
        {
            auto s = m_Data.tm_sec;
            if (s == 60)
                return 0;
            return s;
        }

        inline int64_t TotalSeconds() const
        {
            auto copy = m_Data;
            return mktime(&copy);
        }

        inline static TimeSpan FromSeconds(time_t seconds)
        {
            return TimeSpan(seconds);
        }
    };
} // namespace FE
