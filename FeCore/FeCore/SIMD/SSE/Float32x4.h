#pragma once
#include <FeCore/SIMD/SIMDIntrin.h>

#ifdef FE_SSE3_SUPPORTED
namespace FE
{
    namespace SIMD
    {
        namespace SSE
        {
            // separate definitions and declarations, so that compiler explorer will show assembly for this code

            struct Float32x4
            {
                inline static constexpr size_t ElementCount = 4;

                __m128 Data;

                Float32x4() = default;

                FE_FINLINE Float32x4(__m128 value);

                FE_FINLINE Float32x4(float value);

                FE_FINLINE Float32x4(float x, float y, float z);

                FE_FINLINE Float32x4(float x, float y, float z, float w);

                FE_FINLINE static Float32x4 GetZero() noexcept;

                template<size_t F3, size_t F2, size_t F1, size_t F0>
                FE_FINLINE static Float32x4 Shuffle(Float32x4 a, Float32x4 b) noexcept;

                template<size_t F3, size_t F2, size_t F1, size_t F0>
                FE_FINLINE Float32x4 Shuffle() const noexcept;

                template<size_t I>
                FE_FINLINE Float32x4 Broadcast() const noexcept;

                template<size_t I>
                FE_FINLINE float Select() const noexcept;

                template<>
                FE_FINLINE float Select<0>() const noexcept;

                template<size_t I>
                FE_FINLINE Float32x4 Replace(Float32x4 other) const noexcept;

                template<size_t I>
                FE_FINLINE Float32x4 Replace(float value) const noexcept;

                FE_FINLINE Float32x4 Floor() const noexcept;

                FE_FINLINE Float32x4 Ceil() const noexcept;

                FE_FINLINE Float32x4 Round() const noexcept;

                FE_FINLINE static Float32x4 Min(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 Max(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 CompareEq(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 CompareNeq(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 CompareGt(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 CompareLt(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 CompareGe(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static Float32x4 CompareLe(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE static bool CompareAllEq(Float32x4 a, Float32x4 b, uint32_t mask) noexcept;

                FE_FINLINE static bool CompareAllNeq(Float32x4 a, Float32x4 b, uint32_t mask) noexcept;

                FE_FINLINE static bool CompareAllGt(Float32x4 a, Float32x4 b, uint32_t mask) noexcept;

                FE_FINLINE static bool CompareAllLt(Float32x4 a, Float32x4 b, uint32_t mask) noexcept;

                FE_FINLINE static bool CompareAllGe(Float32x4 a, Float32x4 b, uint32_t mask) noexcept;

                FE_FINLINE static bool CompareAllLe(Float32x4 a, Float32x4 b, uint32_t mask) noexcept;

                FE_FINLINE friend Float32x4 operator+(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE friend Float32x4 operator-(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE friend Float32x4 operator*(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE friend Float32x4 operator/(Float32x4 a, Float32x4 b) noexcept;

                FE_FINLINE Float32x4 Abs() const noexcept;

                FE_FINLINE Float32x4 Inverse() const noexcept;

                FE_FINLINE Float32x4 InverseApprox() const noexcept;
            };

            FE_FINLINE Float32x4::Float32x4(__m128 value)
                : Data(value)
            {
            }

            FE_FINLINE Float32x4::Float32x4(float value)
                : Data(_mm_set_ps1(value))
            {
            }

            FE_FINLINE Float32x4::Float32x4(float x, float y, float z)
                : Data(_mm_set_ps(0, z, y, x))
            {
            }

            FE_FINLINE Float32x4::Float32x4(float x, float y, float z, float w)
                : Data(_mm_set_ps(w, z, y, x))
            {
            }

            FE_FINLINE Float32x4 Float32x4::GetZero() noexcept
            {
                return _mm_setzero_ps();
            }

            template<size_t F3, size_t F2, size_t F1, size_t F0>
            FE_FINLINE Float32x4 Float32x4::Shuffle(Float32x4 a, Float32x4 b) noexcept
            {
                static_assert(F3 < 4 && F2 < 4 && F1 < 4 && F0 < 4);
                return _mm_shuffle_ps(a.Data, b.Data, _MM_SHUFFLE(F3, F2, F1, F0));
            }

            template<size_t F3, size_t F2, size_t F1, size_t F0>
            FE_FINLINE Float32x4 Float32x4::Shuffle() const noexcept
            {
                static_assert(F3 < 4 && F2 < 4 && F1 < 4 && F0 < 4);
                return Shuffle<F3, F2, F1, F0>(*this, *this);
            }

            template<size_t I>
            FE_FINLINE Float32x4 Float32x4::Broadcast() const noexcept
            {
                static_assert(I < 4);
                return Shuffle<I, I, I, I>();
            }

            template<size_t I>
            FE_FINLINE float Float32x4::Select() const noexcept
            {
                static_assert(I < 4);
                return _mm_cvtss_f32(Broadcast<I>());
            }

            template<>
            FE_FINLINE float Float32x4::Select<0>() const noexcept
            {
                return _mm_cvtss_f32(Data);
            }

            template<size_t I>
            FE_FINLINE Float32x4 Float32x4::Replace(Float32x4 other) const noexcept
            {
#    ifdef FE_SSE41_SUPPORTED
                return _mm_blend_ps(Data, other.Data, 1 << I);
#    else
#        error unimplemented
#    endif
            }

            template<size_t I>
            FE_FINLINE Float32x4 Float32x4::Replace(float value) const noexcept
            {
                return Replace<I>(Float32x4(value));
            }

            FE_FINLINE Float32x4 Float32x4::Floor() const noexcept
            {
#    ifdef FE_SSE41_SUPPORTED
                return _mm_floor_ps(Data);
#    else
#        error unimplemented
#    endif
            }

            FE_FINLINE Float32x4 Float32x4::Ceil() const noexcept
            {
#    ifdef FE_SSE41_SUPPORTED
                return _mm_ceil_ps(Data);
#    else
#        error unimplemented
#    endif
            }

            FE_FINLINE Float32x4 Float32x4::Round() const noexcept
            {
#    ifdef FE_SSE41_SUPPORTED
                return _mm_round_ps(Data, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
#    else // TODO: implement this using scalar version
#        error unimplemented
#    endif
            }

            FE_FINLINE Float32x4 Float32x4::Min(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_min_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::Max(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_max_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::CompareEq(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_cmpeq_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::CompareNeq(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_cmpneq_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::CompareGt(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_cmpgt_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::CompareLt(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_cmplt_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::CompareGe(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_cmpge_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::CompareLe(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_cmple_ps(a.Data, b.Data);
            }

            FE_FINLINE bool Float32x4::CompareAllEq(Float32x4 a, Float32x4 b, uint32_t mask) noexcept
            {
                auto cmp = _mm_castps_si128(CompareNeq(a, b).Data);
                return (_mm_movemask_epi8(cmp) & mask) == 0;
            }

            FE_FINLINE bool Float32x4::CompareAllNeq(Float32x4 a, Float32x4 b, uint32_t mask) noexcept
            {
                return !CompareAllEq(a, b, mask);
            }

            FE_FINLINE bool Float32x4::CompareAllGt(Float32x4 a, Float32x4 b, uint32_t mask) noexcept
            {
                auto cmp = _mm_castps_si128(CompareLe(a, b).Data);
                return (_mm_movemask_epi8(cmp) & mask) == 0;
            }

            FE_FINLINE bool Float32x4::CompareAllLt(Float32x4 a, Float32x4 b, uint32_t mask) noexcept
            {
                auto cmp = _mm_castps_si128(CompareGe(a, b).Data);
                return (_mm_movemask_epi8(cmp) & mask) == 0;
            }

            FE_FINLINE bool Float32x4::CompareAllGe(Float32x4 a, Float32x4 b, uint32_t mask) noexcept
            {
                auto cmp = _mm_castps_si128(CompareLt(a, b).Data);
                return (_mm_movemask_epi8(cmp) & mask) == 0;
            }

            FE_FINLINE bool Float32x4::CompareAllLe(Float32x4 a, Float32x4 b, uint32_t mask) noexcept
            {
                auto cmp = _mm_castps_si128(CompareGt(a, b).Data);
                return (_mm_movemask_epi8(cmp) & mask) == 0;
            }

            FE_FINLINE Float32x4 operator+(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_add_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 operator-(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_sub_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 operator*(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_mul_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 operator/(Float32x4 a, Float32x4 b) noexcept
            {
                return _mm_div_ps(a.Data, b.Data);
            }

            FE_FINLINE Float32x4 Float32x4::Abs() const noexcept
            {
                static const auto signMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
                return _mm_andnot_ps(signMask, Data);
            }

            FE_FINLINE Float32x4 Float32x4::Inverse() const noexcept
            {
                return Float32x4(1.0f) / *this;
            }

            FE_FINLINE Float32x4 Float32x4::InverseApprox() const noexcept
            {
                return _mm_rcp_ps(Data);
            }
        } // namespace SSE

    } // namespace SIMD
} // namespace FE
#endif