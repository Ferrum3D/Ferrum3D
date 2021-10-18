#pragma once
#include <csignal>
#include <stdexcept>

#if defined NDEBUG
#    define FE_RELEASE 1
#else
#    define FE_DEBUG 1
#endif

#define FE_MAKE_STR(txt) #txt

#if defined __clang__
#    define FE_COMPILER_CLANG 1

#    if defined _MSC_VER
#        define FE_COMPILER_MS_CLANG 1
#    endif

#    define FE_PUSH_MSVC_WARNING(...)
#    define FE_POP_MSVC_WARNING

#    define FE_PUSH_CLANG_WARNING(warn) _Pragma("clang diagnostic push") _Pragma(FE_MAKE_STR(clang diagnostic ignored warn))
#    define FE_POP_CLANG_WARNING _Pragma("clang diagnostic pop")

#    define FE_FUNCSIG __PRETTY_FUNCTION__

#    ifndef FE_FINLINE
#        define FE_FINLINE inline
#    endif
#elif defined _MSC_VER
#    define FE_COMPILER_MSVC 1

#    define FE_PUSH_MSVC_WARNING(warn) __pragma(warning(push)) __pragma(warning(disable : warn))
#    define FE_POP_MSVC_WARNING __pragma(warning(pop))

#    define FE_PUSH_CLANG_WARNING(...)
#    define FE_POP_CLANG_WARNING

#    define FE_FUNCSIG __FUNCSIG__

#    ifndef FE_FINLINE
#        define FE_FINLINE __forceinline
#    endif
#endif

#if FE_COMPILER_MSVC || FE_COMPILER_MS_CLANG
#    define FE_DEBUGBREAK __debugbreak()
#else
#    define FE_DEBUGBREAK raise(SIGTRAP)
#endif
