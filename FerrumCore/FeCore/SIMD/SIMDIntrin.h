#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Base/Platform.h>
#ifdef FE_SSE3_SUPPORTED
#    include <emmintrin.h>
#    include <xmmintrin.h>
#endif
#ifdef FE_SSE41_SUPPORTED
#    include <smmintrin.h>
#endif
