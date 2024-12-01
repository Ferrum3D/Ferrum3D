#pragma once
#include <csignal>

#if defined _WIN32 || defined _WIN64 || defined _WINDOWS
#    define FE_PLATFORM_WINDOWS 1

#    define FE_DLL_EXTENSION ".dll"
#    define FE_EXE_EXTENSION ".exe"
#    define FE_PATH_SEPARATOR '\\'
#    define FE_OS_NAME "Windows"

#    define FE_BYTE_SWAP_UINT16(value) _byteswap_ushort(value)
#    define FE_BYTE_SWAP_UINT32(value) _byteswap_ulong(value)
#    define FE_BYTE_SWAP_UINT64(value) _byteswap_uint64(value)

#    define FE_DLL_EXPORT __declspec(dllexport)
#    define FE_DLL_IMPORT __declspec(dllimport)
#elif defined __linux__
#    define FE_PLATFORM_LINUX 1

#    define FE_DLL_EXTENSION ".so"
#    define FE_EXE_EXTENSION ""
#    define FE_PATH_SEPARATOR '/'
#    define FE_OS_NAME "Linux"

#    define FE_BYTE_SWAP_UINT16(value) __builtin_bswap16(value)
#    define FE_BYTE_SWAP_UINT32(value) __builtin_bswap32(value)
#    define FE_BYTE_SWAP_UINT64(value) __builtin_bswap64(value)

#    define FE_DLL_EXPORT __attribute__((visibility("default")))
#    define FE_DLL_IMPORT __attribute__((visibility("default")))
#else
#    error Unsupported platform
#endif
