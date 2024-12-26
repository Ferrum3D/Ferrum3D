#pragma once
#include <FeCore/Logging/Logger.h>

namespace FE::Trace
{
    namespace Platform
    {
        void AssertionReport(SourceLocation location, StringSlice message);
    }


    //! @brief Report a recoverable error.
    //!
    //! This function writes a message to currently registered logger with LogSeverity::Error.
    //! Doesn't crash or break in debugger.
    template<class... TArgs>
    inline void ReportError(LogFormatString fmt, TArgs&&... args)
    {
        Env::GetServiceProvider()->ResolveRequired<Logger>()->LogError(fmt, std::forward<TArgs>(args)...);
    }


    //! @brief Report a critical error.
    //!
    //! This function writes a message to currently registered logger with LogSeverity::Critical.
    //! Then does nothing in development builds. Use FE_DebugBreak() after calling this function
    //! to catch the error in the debugger.
    //! In production builds always crashes and never returns.
    template<class... TArgs>
    inline void ReportCritical(LogFormatString fmt, TArgs&&... args)
    {
        // TODO: We need at least three different types of builds:
        //   1. Debug builds;
        //   2. Development builds (release build with developer tools enabled);
        //   3. Production builds.
        //
        // In this function we would want to crash immediately in production builds.
        if (festd::expected result = Env::GetServiceProvider()->Resolve<Logger>())
        {
            result.value()->LogCritical(fmt, std::forward<TArgs>(args)...);
        }
        else
        {
            // The logger has not been initialized yet.
            Platform::AssertionReport(fmt.m_location, Fmt::FixedFormat(fmt.m_value, std::forward<TArgs>(args)...));
        }
    }


#define FE_Assert(expression)                                                                                                    \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expression))                                                                                                       \
        {                                                                                                                        \
            ::FE::Trace::ReportCritical("{}", "Assertion failure: \"" #expression "\"");                                         \
            FE_DebugBreak();                                                                                                     \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)


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


#define FE_Verify(expression) FE_Assert(expression)


#if FE_DEBUG
#    define FE_AssertDebug(expression) FE_Assert(expression)
#    define FE_AssertMsgDebug(expression, ...) FE_AssertMsg(expression, __VA_ARGS__)
#    define FE_VerifyDebug(expression) FE_Verify(expression)
#else
#    define FE_AssertDebug(expression)                                                                                           \
        do                                                                                                                       \
        {                                                                                                                        \
        }                                                                                                                        \
        while (0)
#    define FE_AssertMsgDebug(expression, ...)                                                                                   \
        do                                                                                                                       \
        {                                                                                                                        \
        }                                                                                                                        \
        while (0)
#    define FE_VerifyDebug(expression) ((void)expression)
#endif
} // namespace FE::Trace
