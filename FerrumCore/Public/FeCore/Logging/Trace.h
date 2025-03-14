#pragma once
#include <FeCore/Base/Assert.h>
#include <FeCore/Logging/Logger.h>

namespace FE::Trace::Internal
{
    template<class... TArgs>
    void AssertionReportFormatted(const LogFormatString fmt, TArgs&&... args)
    {
        const festd::fixed_string message = Fmt::FixedFormat(fmt.m_value, festd::forward<TArgs>(args)...);
        AssertionReport(fmt.m_location, message.data(), message.size(), false);
    }
} // namespace FE::Trace::Internal


#define FE_AssertMsg(expression, ...)                                                                                            \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expression))                                                                                                       \
        {                                                                                                                        \
            ::FE::Trace::Internal::AssertionReportFormatted(__VA_ARGS__);                                                        \
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
