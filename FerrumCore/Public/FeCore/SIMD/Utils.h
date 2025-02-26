#pragma once
#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>

namespace FE::SIMD
{
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE __m128 FE_VECTORCALL DotProduct(const __m128 lhs, __m128 rhs)
    {
        const __m128 mul = _mm_mul_ps(lhs, rhs);
        const __m128 t0 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(0, 0, 3, 2));
        const __m128 t1 = _mm_add_ps(mul, t0);
        const __m128 t2 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(0, 0, 0, 1));
        return _mm_add_ps(t1, t2);
    }
} // namespace FE::SIMD
