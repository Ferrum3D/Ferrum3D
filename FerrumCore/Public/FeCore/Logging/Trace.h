#pragma once
#include <FeCore/Base/Assert.h>
#include <FeCore/Logging/Logger.h>

namespace FE::Trace
{
    //! @brief Report a critical error.
    //!
    //! This function notifies currently registered assertion handlers, such as loggers.
    //! Then does nothing in development builds. Use FE_DebugBreak() after calling this function
    //! to catch the error in the debugger.
    //! In production builds always crashes and never returns.
    template<class... TArgs>
    void ReportCritical(const LogFormatString fmt, TArgs&&... args)
    {
        const festd::fixed_string message = Fmt::FixedFormat(fmt.m_value, std::forward<TArgs>(args)...);
        AssertionReport(fmt.m_location, message.data(), message.size(), false);
    }


#define FE_AssertMsg(expression, ...)                                                                                            \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expression))                                                                                                       \
        {                                                                                                                        \
            ::FE::Trace::ReportCritical(__VA_ARGS__);                                                                            \
            FE_DebugBreak();                                                                                                     \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)


#if FE_DEBUG
#    define FE_AssertMsgDebug(expression, ...) FE_AssertMsg(expression, __VA_ARGS__)
#else
#    define FE_AssertMsgDebug(expression, ...)                                                                                   \
        do                                                                                                                       \
        {                                                                                                                        \
        }                                                                                                                        \
        while (0)
#endif
} // namespace FE::Trace
