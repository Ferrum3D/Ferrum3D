#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE
{
    namespace Platform
    {
        //! @brief Low-level assertion report: uses platform-provided functionality to report a critical error.
        //!
        //! @param sourceLocation Location in the source file the error originates from.
        //! @param message        Pointer to the error message encoded in UTF-8.
        //! @param messageSize    The size of the error message in bytes.
        void AssertionReport(SourceLocation sourceLocation, const char* message, uint32_t messageSize);
    } // namespace Platform


    namespace Trace
    {
        //! @brief The maximum number of assertion handlers that can be registered simultaneously.
        inline constexpr uint32_t kMaxAssertionHandlers = 32;


        //! @brief Assertion handler identifier used to unregister assertion handlers.
        struct AssertionHandlerToken final : public TypedHandle<AssertionHandlerToken, uint32_t>
        {
        };


        //! @brief Assertion callback function.
        using AssertionHandler = void (*)(SourceLocation sourceLocation, const char* message, uint32_t messageSize);


        //! @brief Registers the provided callback as an assertion handler and returns the handler's token.
        AssertionHandlerToken RegisterAssertionHandler(AssertionHandler handler);


        //! @brief Unregisters the assertion handler associated with the provided token.
        void UnregisterAssertionHandler(AssertionHandlerToken token);


        //! @brief High level assertion report: uses user-provided functionality to report a critical error.
        //!
        //! This function notifies the assertion handlers registered using Trace::RegisterAssertionHandler and
        //! calls Platform::AssertionReport.
        //!
        //! If none of the handler were registered, this function's behavior is the same as that of Platform::AssertionReport.
        //!
        //! @param sourceLocation Location in the source file the error originates from.
        //! @param message        Pointer to the error message encoded in UTF-8.
        //! @param messageSize    The size of the error message in bytes.
        void AssertionReport(SourceLocation sourceLocation, const char* message, uint32_t messageSize);
    } // namespace Trace
} // namespace FE


#define FE_Assert_Impl(expression, message)                                                                                      \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expression))                                                                                                       \
        {                                                                                                                        \
            constexpr const char* FE_UNIQUE_IDENT(kAssertMessage) = message;                                                     \
            constexpr uint32_t FE_UNIQUE_IDENT(kAssertMessageLength) =                                                           \
                static_cast<uint32_t>(__builtin_strlen(FE_UNIQUE_IDENT(kAssertMessage)));                                        \
            ::FE::Trace::AssertionReport(::FE::SourceLocation::Current(),                                                        \
                                         FE_UNIQUE_IDENT(kAssertMessage),                                                        \
                                         FE_UNIQUE_IDENT(kAssertMessageLength));                                                 \
            FE_DebugBreak();                                                                                                     \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)


#define FE_Assert_1(expression) FE_Assert_Impl(expression, "Assertion Failed: " #expression)
#define FE_Assert_2(expression, message) FE_Assert_Impl(expression, "Assertion Failed: " #expression " (" message ")")

#define FE_Assert(...) FE_MACRO_SPECIALIZE(FE_Assert, __VA_ARGS__)
#define FE_Verify(...) FE_MACRO_SPECIALIZE(FE_Assert, __VA_ARGS__)

#if FE_DEBUG
#    define FE_AssertDebug(...) FE_Assert(__VA_ARGS__)
#    define FE_VerifyDebug(...) FE_Verify(__VA_ARGS__)
#else
#    define FE_AssertDebug(...)                                                                                                  \
        do                                                                                                                       \
        {                                                                                                                        \
        }                                                                                                                        \
        while (0)
#    define FE_VerifyDebug(expression, ...) ((void)(expression))
#endif
