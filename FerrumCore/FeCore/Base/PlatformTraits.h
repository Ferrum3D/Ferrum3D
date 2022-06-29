#pragma once
#include <csignal>
#include <malloc.h>

#if defined _WIN32 || defined _WIN64 || defined _WINDOWS
#    define FE_WINDOWS 1

#    define FE_DLL_EXTENSION ".dll"
#    define FE_EXE_EXTENSION ".exe"
#    define FE_PATH_SEPARATOR '\\'
#    define FE_OS_NAME "Windows"

#    define FE_ALIGNED_MALLOC(size, alignment) _aligned_malloc(size, alignment)
#    define FE_ALIGNED_FREE(ptr) _aligned_free(ptr)

#    define FE_DLL_EXPORT __declspec(dllexport)
#    define FE_DLL_IMPORT __declspec(dllimport)

#elif defined __linux__
#    define FE_LINUX 1

#    define FE_DLL_EXTENSION ".so"
#    define FE_EXE_EXTENSION ""
#    define FE_PATH_SEPARATOR '/'
#    define FE_OS_NAME "Linux"

#    define FE_ALIGNED_MALLOC(size, alignment) ::memalign(alignment, size)
#    define FE_ALIGNED_FREE(ptr) ::free(ptr)

#    define FE_DLL_EXPORT __attribute__((visibility("default")))
#    define FE_DLL_IMPORT __attribute__((visibility("default")))

#else
#    error Unsupported platform
#endif
