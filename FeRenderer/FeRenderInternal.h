#pragma once

#define ENGINE_DLL 1

#if !defined(PLATFORM_WIN32) && defined(FE_WINDOWS)
#    define PLATFORM_WIN32 1
#endif

#if !defined(PLATFORM_LINUX) && defined(FE_LINUX)
#    define PLATFORM_LINUX 1
#endif

#ifndef ENGINE_DLL
#    define ENGINE_DLL 1
#endif

#if defined(FE_WINDOWS)
#    define D3D11_SUPPORTED 1
#endif

#if defined(FE_WINDOWS)
#    define D3D12_SUPPORTED 1
#endif

#if 1
#    define GL_SUPPORTED 1
#endif

#if defined(FE_WINDOWS) || defined(FE_LINUX)
#    define VULKAN_SUPPORTED 1
#endif
