#pragma once
#include <signal.h>
#include <stdexcept>

#if defined NDEBUG
#    define FE_RELEASE 1
#else
#    define FE_DEBUG 1
#endif

#if defined _MSC_VER
#    define FE_COMPILER_MSVC 1

#    define FE_FUNCSIG __FUNCSIG__
#    define FE_FINLINE __forceinline

#    define FE_DEBUGBREAK __debugbreak()
#elif defined __clang__
#    define FE_COMPILER_CLANG 1

#    define FE_FUNCSIG __PRETTY_FUNCTION__
#    define FE_FINLINE inline

#    define FE_DEBUGBREAK raise(SIGTRAP)
#endif
