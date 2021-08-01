#pragma once
#include <Console/ConsoleLogger.h>

namespace FE
{
#define FE_LOG_MESSAGE(...) ::FE::Singleton<::FE::Debug::IConsoleLogger>::Get()->LogMessage(__VA_ARGS__)
#define FE_LOG_ERROR(...) ::FE::Singleton<::FE::Debug::IConsoleLogger>::Get()->LogError(__VA_ARGS__)
#define FE_LOG_WARNING(...) ::FE::Singleton<::FE::Debug::IConsoleLogger>::Get()->LogError(__VA_ARGS__)

#define FE_ASSERT(_Stmt)                                                                                                         \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(_Stmt))                                                                                                            \
        {                                                                                                                        \
            FE_LOG_ERROR("Assertion failed in file {} at line {}", __FILE__, __LINE__);                                          \
            FE_DEBUGBREAK;                                                                                                        \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

#define FE_ASSERT_MSG(_Stmt, ...)                                                                                                \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(_Stmt))                                                                                                            \
        {                                                                                                                        \
            FE_LOG_ERROR("Error in file {} at line {}: {}", __FILE__, __LINE__, ::FE::Fmt::Format(__VA_ARGS__));                 \
            FE_DEBUGBREAK;                                                                                                        \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)
} // namespace FE
