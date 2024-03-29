#pragma once
#include <FeCore/Base/Platform.h>

#if FE_WINDOWS
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    include <atlbase.h>
#    include <atlcom.h>
#    include <guiddef.h>

#    undef CopyMemory
#    undef GetObject
#    undef CreateWindow
#    undef MemoryBarrier
#else
#    error Unsupported platform
#endif
