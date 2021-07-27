#pragma once
#include <SIMD/SIMDIntrin.h>

#ifdef FE_SSE3_SUPPORTED
namespace FE::SIMD::SSE
{
    struct Float32x4
    {
        using TPacked  = __m128;
        using TElement = float;
        inline static constexpr size_t ElementCount = 4;

        TPacked Data;

        FE_FINLINE Float32x4(TPacked value)
            : Data(value)
        {
        }

        FE_FINLINE Float32x4(TElement value)
            : Data(_mm_set_ps1(value))
        {
        }

        FE_FINLINE Float32x4(TElement x, TElement y, TElement z, TElement w)
            : Data(_mm_set_ps(x, y, z, w))
        {
        }

        FE_FINLINE static Float32x4 GetZero() noexcept
        {
            return _mm_setzero_ps();
        }

        template<size_t F3, size_t F2, size_t F1, size_t F0>
        FE_FINLINE static Float32x4 Shuffle(Float32x4 a, Float32x4 b) noexcept
        {
            static_assert(F3 < 4 && F2 < 4 && F1 < 4 && F0 < 4);
            return _mm_shuffle_ps(a.Data, b.Data, _MM_SHUFFLE(F3, F2, F1, F0));
        }

        template<size_t F3, size_t F2, size_t F1, size_t F0>
        FE_FINLINE Float32x4 Shuffle() const noexcept
        {
            static_assert(F3 < 4 && F2 < 4 && F1 < 4 && F0 < 4);
            return Shuffle<F3, F2, F1, F0>(*this, *this);
        }

        template<size_t I>
        FE_FINLINE Float32x4 Broadcast() const noexcept
        {
            static_assert(I < 4);
            return Shuffle<I, I, I, I>();
        }

        template<size_t I>
        FE_FINLINE TElement Select() const noexcept
        {
            static_assert(I < 4);
            return _mm_cvtss_f32(Broadcast<I>());
        }

        template<>
        FE_FINLINE TElement Select<0>() const noexcept
        {
            return _mm_cvtss_f32(Data);
        }

        template<size_t I>
        FE_FINLINE Float32x4 Replace(Float32x4 other) const noexcept
        {
#    ifdef FE_SSE41_SUPPORTED
            return _mm_blend_ps(Data, other.Data, 1 << I);
#    else
#        error unimplemented
#    endif
        }

        template<size_t I>
        FE_FINLINE Float32x4 Replace(TElement value) const noexcept
        {
            return Replace<I>(Float32x4(value));
        }

        FE_FINLINE Float32x4 Floor() const noexcept
        {
#    ifdef FE_SSE41_SUPPORTED
            return _mm_floor_ps(Data);
#    else
#        error unimplemented
#    endif
        }

        FE_FINLINE Float32x4 Ceil() const noexcept
        {
#    ifdef FE_SSE41_SUPPORTED
            return _mm_ceil_ps(Data);
#    else
#        error unimplemented
#    endif
        }

        FE_FINLINE Float32x4 Round() const noexcept
        {
#    ifdef FE_SSE41_SUPPORTED
            _mm_round_ps(Data, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
#    else // TODO: implement this using scalar version
#        error unimplemented
#    endif
        }

        FE_FINLINE static Float32x4 Min(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_min_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 Max(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_max_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 CopmareEq(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_cmpeq_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 CopmareNeq(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_cmpneq_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 CopmareGt(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_cmpgt_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 CopmareLt(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_cmplt_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 CopmareGe(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_cmpge_ps(a.Data, b.Data);
        }

        FE_FINLINE static Float32x4 CopmareLe(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_cmple_ps(a.Data, b.Data);
        }

        FE_FINLINE friend Float32x4 operator+(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_add_ps(a.Data, b.Data);
        }

        FE_FINLINE friend Float32x4 operator-(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_sub_ps(a.Data, b.Data);
        }

        FE_FINLINE friend Float32x4 operator*(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_mul_ps(a.Data, b.Data);
        }

        FE_FINLINE friend Float32x4 operator/(Float32x4 a, Float32x4 b) noexcept
        {
            return _mm_div_ps(a.Data, b.Data);
        }

        FE_FINLINE Float32x4 Abs() const noexcept
        {
            static const auto signMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
            return _mm_andnot_ps(signMask, Data);
        }

        FE_FINLINE Float32x4 Inverse() const noexcept
        {
            return Float32x4(1.0f) / *this;
        }

        FE_FINLINE Float32x4 InverseApprox() const noexcept
        {
            return _mm_rcp_ps(Data);
        }
    };
} // namespace FE::SIMD::SSE
#endif
