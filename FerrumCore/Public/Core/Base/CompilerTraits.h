#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <type_traits>

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

#define FE_MACRO_ARGUMENT_COUNT(...) FE_MACRO_ARGUMENT_NUMBERS(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define FE_MACRO_ARGUMENT_NUMBERS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, N, ...) N

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


#if (FE_COMPILER_MSVC || FE_COMPILER_MS_CLANG) && !FE_CODEGEN
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
