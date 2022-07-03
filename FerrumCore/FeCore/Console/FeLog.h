#pragma once
#include <FeCore/Console/ConsoleLogger.h>

namespace FE
{
    //! brief Log a message using currently registered instance of FE::Debug::IConsoleLogger.
#define FE_LOG_MESSAGE(...) ::FE::SharedInterface<::FE::Debug::IConsoleLogger>::Get()->LogMessage(__VA_ARGS__)

    //! \brief Log an error using currently registered instance of FE::Debug::IConsoleLogger.
#define FE_LOG_ERROR(...) ::FE::SharedInterface<::FE::Debug::IConsoleLogger>::Get()->LogError(__VA_ARGS__)

    //! \brief Log a warning using currently registered instance of FE::Debug::IConsoleLogger.
#define FE_LOG_WARNING(...) ::FE::SharedInterface<::FE::Debug::IConsoleLogger>::Get()->LogWarning(__VA_ARGS__)

    //! \brief Log an error and break the attached debugger if an expression was false. Will crash in release builds.
#define FE_ASSERT(expr)                                                                                                          \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expr))                                                                                                             \
        {                                                                                                                        \
            FE_LOG_ERROR("Assertion (" #expr ") failed in file " __FILE__ " at line {}", __LINE__);                              \
            FE_DEBUGBREAK;                                                                                                       \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

    //! \brief Log an error and break the attached debugger if an expression was false. Will crash in release builds.
#define FE_ASSERT_MSG(_Stmt, ...)                                                                                                \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(_Stmt))                                                                                                            \
        {                                                                                                                        \
            FE_LOG_ERROR("Error in file " __FILE__ " at line {}: {}", __LINE__, ::FE::Fmt::Format(__VA_ARGS__));                 \
            FE_DEBUGBREAK;                                                                                                       \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

    //! \brief Equivalent to FE_ASSERT_MSG(false, ...)
#define FE_UNREACHABLE(...) FE_ASSERT_MSG(false, __VA_ARGS__)
} // namespace FE
