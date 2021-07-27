#pragma once
#include <signal.h>
#include <stdexcept>
#include <string>
#include <memory>

#ifdef _WIN32
#    define FE_WINDOWS 1
#    define FE_DEBUGBREAK __debugbreak();
#elif defined(__linux__)
#    define FE_LINUX 1
#    define FE_DEBUGBREAK raise(SIGTRAP);
#endif

#ifdef FE_WINDOWS
#    ifdef FERRUMCORE_EXPORTS
#        define FE_CORE_API __declspec(dllexport)
#    else
#        define FE_CORE_API __declspec(dllimport)
#    endif
#endif

namespace FE
{
#ifdef _MSC_VER
#    define FE_FUNCNAME __FUNCSIG__
#    define FE_FINLINE __forceinline

#    define FE_ALIGNED_MALLOC(size, alignment) _aligned_malloc(size, alignment)
#    define FE_ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
#    define FE_FUNCNAME __PRETTY_FUNCTION__
#endif
} // namespace FE

#ifdef NDEBUG
#    define FE_RELEASE 1

#    define FE_DEBUGBREAK throw ::std::runtime_error("Fatal error");
#else
#    define FE_DEBUG 1

#    if defined(FE_WINDOWS)
#        define FE_DEBUGBREAK __debugbreak();
#    elif defined(FE_LINUX)
#        define FE_DEBUGBREAK raise(SIGTRAP);
#    endif
#endif
