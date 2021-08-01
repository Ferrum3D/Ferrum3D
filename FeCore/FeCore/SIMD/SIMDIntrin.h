#pragma once
#include <FeCore/Utils/Platform.h>
#ifdef FE_SSE3_SUPPORTED
#    include <emmintrin.h>
#    include <xmmintrin.h>
#endif
#ifdef FE_SSE41_SUPPORTED
#    include <smmintrin.h>
#endif
