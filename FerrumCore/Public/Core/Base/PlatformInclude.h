#pragma once
#include <Core/Base/PlatformTraits.h>

#if FE_PLATFORM_WINDOWS
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    include <atlbase.h>
#    include <atlcom.h>
#    include <guiddef.h>

#    undef near
#    undef far

#    undef CopyMemory
#    undef CreateWindow
#    undef CreateFile
#    undef GetObject
#    undef MemoryBarrier
#    undef GetCurrentDirectory
#    undef SetCurrentDirectory
#else
#    error Unsupported platform
#endif
