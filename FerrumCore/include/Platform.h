#pragma once
#include <signal.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#	define FE_WINDOWS 1
#	define FE_DEBUGBREAK __debugbreak();
#elif defined(__linux__)
#	define FE_LINUX 1
#	define FE_DEBUGBREAK raise(SIGTRAP);
#endif

#ifdef FE_WINDOWS
#	ifdef FERRUMCORE_EXPORTS
#		define FE_CORE_API __declspec(dllexport)
#	else
#		define FE_CORE_API __declspec(dllimport)
#	endif
#endif

namespace Ferrum
{
#if FE_WINDOWS
#	define FE_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#	define FE_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define FE_DEBUG_FUNC FeFnCallstackHandle FE_FN_CS_HANDLE(FeJobSystem::Get().GetCurrentThreadId(), __PRETTY_FUNCTION__, FE_FILENAME, __LINE__);
}

#ifdef NDEBUG
#	define FE_RELEASE 1

#	define FE_DEBUGBREAK throw ::std::runtime_error("Fatal error");
#else
#	define FE_DEBUG 1

#	if defined(FE_WINDOWS)
#		define FE_DEBUGBREAK __debugbreak();
#	elif defined(FE_LINUX)
#		define FE_DEBUGBREAK raise(SIGTRAP);
#	endif
#endif
