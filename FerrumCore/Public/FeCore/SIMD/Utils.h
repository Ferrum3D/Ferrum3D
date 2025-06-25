#pragma once
#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>

namespace FE::SIMD
{
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE __m128 FE_VECTORCALL DotProduct(const __m128 lhs, const __m128 rhs)
    {
        const __m128 mul = _mm_mul_ps(lhs, rhs);
        const __m128 t0 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(0, 0, 3, 2));
        const __m128 t1 = _mm_add_ps(mul, t0);
        const __m128 t2 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(0, 0, 0, 1));
        return _mm_add_ps(t1, t2);
    }


    namespace AVX
    {
        inline constexpr uint32_t kByteSize = sizeof(__m256);
        inline constexpr uint32_t kLaneCount = kByteSize / sizeof(float);


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Zero(void* buffer, const uint32_t alignedSize)
        {
            FE_AssertDebug(IsAligned(alignedSize, sizeof(__m256)));

            auto* byteBuffer = static_cast<std::byte*>(buffer);
            for (uint32_t i = 0; i < alignedSize; i += sizeof(__m256))
            {
                _mm256_storeu_ps(reinterpret_cast<float*>(byteBuffer + i), _mm256_setzero_ps());
            }
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Copy(void* dst, const void* src, const uint32_t alignedSize)
        {
            FE_AssertDebug(IsAligned(alignedSize, sizeof(__m256)));

            auto* dstBuffer = static_cast<std::byte*>(dst);
            auto* srcBuffer = static_cast<const std::byte*>(src);
            for (uint32_t i = 0; i < alignedSize; i += sizeof(__m256))
            {
                const __m256 srcVec = _mm256_loadu_ps(reinterpret_cast<const float*>(srcBuffer + i));
                _mm256_storeu_ps(reinterpret_cast<float*>(dstBuffer + i), srcVec);
            }
        }
    } // namespace AVX


    namespace SSE
    {
        inline constexpr uint32_t kByteSize = sizeof(__m128);
        inline constexpr uint32_t kLaneCount = kByteSize / sizeof(float);


        namespace Masks
        {
            inline const __m128 kFloatX = _mm_castsi128_ps(_mm_setr_epi32(UINT32_MAX, 0, 0, 0));
            inline const __m128 kFloatY = _mm_castsi128_ps(_mm_setr_epi32(0, UINT32_MAX, 0, 0));
            inline const __m128 kFloatZ = _mm_castsi128_ps(_mm_setr_epi32(0, 0, UINT32_MAX, 0));
            inline const __m128 kFloatW = _mm_castsi128_ps(_mm_setr_epi32(0, 0, 0, UINT32_MAX));

            inline const __m128 kFloatXYZ = _mm_castsi128_ps(_mm_setr_epi32(UINT32_MAX, UINT32_MAX, UINT32_MAX, 0));
        } // namespace Masks


        namespace Constants
        {
            inline const __m128 kFloat1111 = _mm_set1_ps(1.0f);
            inline const __m128 kFloat0000 = _mm_setzero_ps();

            inline const __m128 kFloat1110 = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f);
        } // namespace Constants


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Zero(void* buffer, const uint32_t alignedSize)
        {
            FE_AssertDebug(IsAligned(alignedSize, sizeof(__m128)));

            auto* byteBuffer = static_cast<std::byte*>(buffer);
            for (uint32_t i = 0; i < alignedSize; i += sizeof(__m128))
            {
                _mm_storeu_ps(reinterpret_cast<float*>(byteBuffer + i), _mm_setzero_ps());
            }
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Copy(void* dst, const void* src, const uint32_t alignedSize)
        {
            FE_AssertDebug(IsAligned(alignedSize, sizeof(__m128)));

            auto* dstBuffer = static_cast<std::byte*>(dst);
            auto* srcBuffer = static_cast<const std::byte*>(src);
            for (uint32_t i = 0; i < alignedSize; i += sizeof(__m128))
            {
                const __m128 srcVec = _mm_loadu_ps(reinterpret_cast<const float*>(srcBuffer + i));
                _mm_storeu_ps(reinterpret_cast<float*>(dstBuffer + i), srcVec);
            }
        }
    } // namespace SSE
} // namespace FE::SIMD
