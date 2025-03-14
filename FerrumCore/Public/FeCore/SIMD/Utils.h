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
        static constexpr uint32_t kByteSize = sizeof(__m256);
        static constexpr uint32_t kLaneCount = kByteSize / sizeof(float);


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
        static constexpr uint32_t kByteSize = sizeof(__m128);
        static constexpr uint32_t kLaneCount = kByteSize / sizeof(float);


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
