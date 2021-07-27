#pragma once
#include <Utils/Platform.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#ifdef FE_SSE41_SUPPORTED
#    ifndef FE_SSE3_SUPPORTED
#        define FE_SSE3_SUPPORTED
#    endif
#    include <smmintrin.h>
#endif
