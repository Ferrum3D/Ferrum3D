#pragma once
#include <FeCore/Utils/CoreUtils.h>

#ifdef FE_WINDOWS
#    ifdef FERRUMRENDERER_EXPORTS
#        define FE_RENDER_API __declspec(dllexport)
#    else
#        define FE_RENDER_API __declspec(dllimport)
#    endif
#endif
