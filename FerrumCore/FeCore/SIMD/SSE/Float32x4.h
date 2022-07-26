#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/SIMD/SIMDIntrin.h>

#ifdef FE_SSE3_SUPPORTED
namespace FE::SIMD::SSE
{
    // Separate definitions and declarations, so that compiler explorer will show assembly for this code.

    //! \brief A wrapper on SSE vector (`__m128`) that provides operator overloading.
    //!
    //! This struct provides interface that hides ugly _mm_XXX(...) functions and makes it easier to work with SIMD.
    //! It also exists to provide scalar (non-SIMD) implementation for older processors.
    //! The provided functions use SSE4.1, but it's planned to conditionally compile them for SSE3 only as an option.
    struct Float32x4
    {
        FE_STRUCT_RTTI(Float32x4, "7A8BB7B4-0841-4D71-A81D-D3F76828F08F");

        inline static constexpr size_t ElementCount = 4; //!< Number of elements in a vector.

        __m128 Data; //!< Underlying SIMD vector type.

        Float32x4() = default;

        FE_FINLINE Float32x4(__m128 value);

        FE_FINLINE Float32x4(Float32 value);

        //! \brief Set Data = { x, y, 0, 0 }
        FE_FINLINE Float32x4(Float32 x, Float32 y);

        //! \brief Set Data = { x, y, z, 0 }
        FE_FINLINE Float32x4(Float32 x, Float32 y, Float32 z);

        //! \brief Set Data = { x, y, z, w }
        FE_FINLINE Float32x4(Float32 x, Float32 y, Float32 z, Float32 w);

        //! \brief Get { 0, 0, 0, 0 }
        FE_FINLINE static Float32x4 GetZero() noexcept;

        template<size_t F3, size_t F2, size_t F1, size_t F0>
        FE_FINLINE static Float32x4 Shuffle(Float32x4 a, Float32x4 b) noexcept;

        template<size_t F3, size_t F2, size_t F1, size_t F0>
        FE_FINLINE Float32x4 Shuffle() const noexcept;

        template<size_t I>
        FE_FINLINE Float32x4 Broadcast() const noexcept;

        template<size_t I>
        FE_FINLINE Float32 Select() const noexcept;

        template<>
        FE_FINLINE Float32 Select<0>() const noexcept;

        template<size_t I>
        FE_FINLINE Float32x4 Replace(Float32x4 other) const noexcept;

        template<size_t I>
        FE_FINLINE Float32x4 Replace(Float32 value) const noexcept;

        FE_FINLINE Float32x4 Floor() const noexcept;

        FE_FINLINE Float32x4 Ceil() const noexcept;

        FE_FINLINE Float32x4 Round() const noexcept;

        FE_FINLINE Float32x4 HorizontalAdd(Float32x4 b) const noexcept;

        FE_FINLINE static Float32x4 Min(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 Max(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 CompareEq(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 CompareNeq(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 CompareGt(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 CompareLt(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 CompareGe(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static Float32x4 CompareLe(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE static bool CompareAllEq(Float32x4 a, Float32x4 b, UInt32 mask) noexcept;

        FE_FINLINE static bool CompareAllNeq(Float32x4 a, Float32x4 b, UInt32 mask) noexcept;

        FE_FINLINE static bool CompareAllGt(Float32x4 a, Float32x4 b, UInt32 mask) noexcept;

        FE_FINLINE static bool CompareAllLt(Float32x4 a, Float32x4 b, UInt32 mask) noexcept;

        FE_FINLINE static bool CompareAllGe(Float32x4 a, Float32x4 b, UInt32 mask) noexcept;

        FE_FINLINE static bool CompareAllLe(Float32x4 a, Float32x4 b, UInt32 mask) noexcept;

        FE_FINLINE friend Float32x4 operator&(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE friend Float32x4 operator|(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE friend Float32x4 operator^(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE friend Float32x4 operator+(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE friend Float32x4 operator-(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE friend Float32x4 operator*(Float32x4 a, Float32x4 b) noexcept;

        FE_FINLINE friend Float32x4 operator/(Float32x4 a, Float32x4 b) noexcept;

        [[nodiscard]] FE_FINLINE Float32x4 Abs() const noexcept;

        [[nodiscard]] FE_FINLINE Float32x4 Inverse() const noexcept;

        [[nodiscard]] FE_FINLINE Float32x4 InverseApprox() const noexcept;

        [[nodiscard]] FE_FINLINE Float32x4 NegateXYZ() const noexcept;

        [[nodiscard]] FE_FINLINE Float32x4 NegateW() const noexcept;

        [[nodiscard]] FE_FINLINE Float32x4 Negate() const noexcept;
    };

    FE_FINLINE Float32x4::Float32x4(__m128 value)
        : Data(value)
    {
    }

    FE_FINLINE Float32x4::Float32x4(Float32 value)
        : Data(_mm_set_ps1(value))
    {
    }

    FE_FINLINE Float32x4::Float32x4(Float32 x, Float32 y)
        : Data(_mm_set_ps(0, 0, y, x))
    {
    }

    FE_FINLINE Float32x4::Float32x4(Float32 x, Float32 y, Float32 z)
        : Data(_mm_set_ps(0, z, y, x))
    {
    }

    FE_FINLINE Float32x4::Float32x4(Float32 x, Float32 y, Float32 z, Float32 w)
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
        // TODO: test this _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(*this), _MM_SHUFFLE(F3, F2, F1, F0)));
        return Shuffle<F3, F2, F1, F0>(*this, *this);
    }

    template<size_t I>
    FE_FINLINE Float32x4 Float32x4::Broadcast() const noexcept
    {
        static_assert(I < 4);
        return Shuffle<I, I, I, I>();
    }

    template<size_t I>
    FE_FINLINE Float32 Float32x4::Select() const noexcept
    {
        static_assert(I < 4);
        return _mm_cvtss_f32(Broadcast<I>());
    }

    template<>
    FE_FINLINE Float32 Float32x4::Select<0>() const noexcept
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
    FE_FINLINE Float32x4 Float32x4::Replace(Float32 value) const noexcept
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

    FE_FINLINE bool Float32x4::CompareAllEq(Float32x4 a, Float32x4 b, UInt32 mask) noexcept
    {
        auto cmp = _mm_castps_si128(CompareNeq(a, b).Data);
        return (_mm_movemask_epi8(cmp) & mask) == 0;
    }

    FE_FINLINE bool Float32x4::CompareAllNeq(Float32x4 a, Float32x4 b, UInt32 mask) noexcept
    {
        return !CompareAllEq(a, b, mask);
    }

    FE_FINLINE bool Float32x4::CompareAllGt(Float32x4 a, Float32x4 b, UInt32 mask) noexcept
    {
        auto cmp = _mm_castps_si128(CompareLe(a, b).Data);
        return (_mm_movemask_epi8(cmp) & mask) == 0;
    }

    FE_FINLINE bool Float32x4::CompareAllLt(Float32x4 a, Float32x4 b, UInt32 mask) noexcept
    {
        auto cmp = _mm_castps_si128(CompareGe(a, b).Data);
        return (_mm_movemask_epi8(cmp) & mask) == 0;
    }

    FE_FINLINE bool Float32x4::CompareAllGe(Float32x4 a, Float32x4 b, UInt32 mask) noexcept
    {
        auto cmp = _mm_castps_si128(CompareLt(a, b).Data);
        return (_mm_movemask_epi8(cmp) & mask) == 0;
    }

    FE_FINLINE bool Float32x4::CompareAllLe(Float32x4 a, Float32x4 b, UInt32 mask) noexcept
    {
        auto cmp = _mm_castps_si128(CompareGt(a, b).Data);
        return (_mm_movemask_epi8(cmp) & mask) == 0;
    }

    FE_FINLINE Float32x4 operator&(Float32x4 a, Float32x4 b) noexcept
    {
        return _mm_and_ps(a.Data, b.Data);
    }

    FE_FINLINE Float32x4 operator|(Float32x4 a, Float32x4 b) noexcept
    {
        return _mm_or_ps(a.Data, b.Data);
    }

    FE_FINLINE Float32x4 operator^(Float32x4 a, Float32x4 b) noexcept
    {
        return _mm_xor_ps(a.Data, b.Data);
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
        static const auto signMask = _mm_castsi128_ps(_mm_set1_epi32(static_cast<Int32>(0x80000000)));
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

    FE_FINLINE Float32x4 Float32x4::NegateXYZ() const noexcept
    {
        alignas(16) static constexpr Int32 values[] = { static_cast<Int32>(0x80000000),
                                                        static_cast<Int32>(0x80000000),
                                                        static_cast<Int32>(0x80000000),
                                                        static_cast<Int32>(0x00000000) };

        static const auto mask = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(values)));
        return *this ^ Float32x4(mask);
    }

    Float32x4 Float32x4::HorizontalAdd(Float32x4 b) const noexcept
    {
        return _mm_hadd_ps(Data, b.Data);
    }

    Float32x4 Float32x4::NegateW() const noexcept
    {
        alignas(16) static constexpr Int32 values[] = { static_cast<Int32>(0x00000000),
                                                        static_cast<Int32>(0x00000000),
                                                        static_cast<Int32>(0x00000000),
                                                        static_cast<Int32>(0x80000000) };

        static const auto mask = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(values)));
        return *this ^ Float32x4(mask);
    }

    Float32x4 Float32x4::Negate() const noexcept
    {
        alignas(16) static constexpr Int32 values[] = { static_cast<Int32>(0x80000000),
                                                        static_cast<Int32>(0x80000000),
                                                        static_cast<Int32>(0x80000000),
                                                        static_cast<Int32>(0x80000000) };

        static const auto mask = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(values)));
        return *this ^ Float32x4(mask);
    }
} // namespace FE::SIMD::SSE
#endif
