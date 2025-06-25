#pragma once
#include <csignal>
#include <stdexcept>

#if defined NDEBUG
#    define FE_RELEASE 1
#else
#    define FE_DEBUG 1
#endif

// TODO: fix this when we have shipping builds
#define FE_SHIPPING 0

#if !FE_SHIPPING
#    define FE_DEVELOPMENT 1
#endif

//! @brief Utility to support "overloading" of function-like macros.
//!
//! Example:
//! @code{.cpp}
//!      #define FOO_1(a) (a)
//!      #define FOO_2(a, b) ((a) + (b))
//!      #define FOO_3(a, b, c) ((a) + (b) + (c))
//!      #define FOO(...) FE_MACRO_SPECIALIZE(FOO, __VA_ARGS__)
//!      static_assert(FOO(1) + FOO(1, 1) + FOO(1, 1, 1) == 6);
//! @endcode
#define FE_MACRO_SPECIALIZE(func, ...) FE_JOIN(func##_, FE_MACRO_ARGUMENT_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define FE_MAKE_STRING(txt) #txt
#define FE_JOIN_IMPL(a, b) a##b
#define FE_JOIN(a, b) FE_JOIN_IMPL(a, b)
#define FE_UNIQUE_IDENT(name) FE_JOIN(name, __LINE__)
#define FE_Unused(x) ((void)(x))

// clang-format off
#define FE_MACRO_ARGUMENT_COUNT(...) FE_MACRO_ARGUMENT_NUMBERS(__VA_ARGS__,                                                      \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33,  \
    32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define FE_MACRO_ARGUMENT_NUMBERS(                                                                                               \
    _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,  \
    _34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,     \
    N,...) N
// clang-format on

//
// We only want to inline FE_ALWAYS_INLINE functions in debug builds.
// MSVC with '/d2Obforceinline' and Clang will inline functions marked with __forceinline (__attribute__((always_inline))),
// so we define FE_FORCE_INLINE as just `inline` in debug.
// In Release builds, however, both FE_FORCE_INLINE and FE_ALWAYS_INLINE
// are defined as `__forceinline` (__attribute__((always_inline))).
//

#if defined __clang__
#    define FE_COMPILER_CLANG 1

#    if defined _MSC_VER
#        define FE_COMPILER_MS_CLANG 1
#    endif

#    define FE_PUSH_MSVC_WARNING(...)
#    define FE_POP_MSVC_WARNING()

#    define FE_PUSH_CLANG_WARNING(warn) _Pragma("clang diagnostic push") _Pragma(FE_MAKE_STRING(clang diagnostic ignored warn))
#    define FE_POP_CLANG_WARNING() _Pragma("clang diagnostic pop")

#    define FE_FUNCSIG __PRETTY_FUNCTION__

#    if FE_DEBUG && !__INTELLISENSE__ && !__JETBRAINS_IDE__
#        define FE_FORCE_INLINE inline
#        define FE_ALWAYS_INLINE __attribute__((always_inline)) inline
#    else
#        define FE_FORCE_INLINE __attribute__((always_inline)) inline
#        define FE_ALWAYS_INLINE __attribute__((always_inline)) inline
#    endif

#    define FE_FORCE_NOINLINE __attribute__((noinline))
#elif defined _MSC_VER
#    define FE_COMPILER_MSVC 1

#    define FE_PUSH_MSVC_WARNING(warn) __pragma(warning(push)) __pragma(warning(disable : warn))
#    define FE_POP_MSVC_WARNING() __pragma(warning(pop))

#    define FE_PUSH_CLANG_WARNING(...)
#    define FE_POP_CLANG_WARNING()

#    define FE_FUNCSIG __FUNCSIG__

#    if FE_DEBUG && !__INTELLISENSE__ && !__JETBRAINS_IDE__
#        define FE_FORCE_INLINE inline
#        define FE_ALWAYS_INLINE __forceinline
#    else
#        define FE_FORCE_INLINE __forceinline
#        define FE_ALWAYS_INLINE __forceinline
#    endif

#    define FE_FORCE_NOINLINE __declspec(noinline)
#endif


#if FE_COMPILER_MSVC || FE_COMPILER_MS_CLANG
#    define FE_DebugBreak() __debugbreak()
#    define FE_VECTORCALL __vectorcall
#    define FE_NO_SECURITY_COOKIE __declspec(safebuffers)
#else
#    define FE_DebugBreak() __builtin_debugtrap()
#    define FE_VECTORCALL
#    define FE_NO_SECURITY_COOKIE
#endif

namespace FE::Build
{
    FE_FORCE_INLINE bool IsDebug()
    {
#if FE_DEBUG
        return true;
#else
        return false;
#endif
    }


    FE_FORCE_INLINE bool IsDevelopment()
    {
        return FE_DEVELOPMENT;
    }


    FE_FORCE_INLINE bool IsShipping()
    {
        return FE_SHIPPING;
    }
} // namespace FE::Build
