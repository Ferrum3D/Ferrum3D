#pragma once
#include <FeCore/Base/Assert.h>
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/Base/CompilerTraits.h>
#include <festd/base.h>

namespace FE
{
    //! @brief Align up an integer.
    //!
    //! @param x         Value to align.
    //! @param alignment Alignment to use.
    template<class T, class TAlignmentType = T>
    FE_FORCE_INLINE constexpr T AlignUp(T x, const TAlignmentType alignment)
    {
        return static_cast<T>((x + (static_cast<T>(alignment) - 1u)) & ~(static_cast<T>(alignment) - 1u));
    }


    //! @brief Align up a pointer.
    //!
    //! @param x         Value to align.
    //! @param alignment Alignment to use.
    template<class T>
    FE_FORCE_INLINE T* AlignUpPtr(T* x, const size_t alignment)
    {
        return reinterpret_cast<T*>(AlignUp(reinterpret_cast<uintptr_t>(x), alignment));
    }


    //! @brief Align up an integer.
    //!
    //! @param x           Value to align.
    //! @tparam TAlignment Alignment to use.
    template<uint32_t TAlignment, class T>
    FE_FORCE_INLINE constexpr T AlignUp(const T x)
    {
        return (x + (static_cast<T>(TAlignment) - 1)) & ~(static_cast<T>(TAlignment) - 1);
    }


    //! @brief Align down an integer.
    //!
    //! @param x         Value to align.
    //! @param alignment Alignment to use.
    template<class T, class TAlignmentType = T>
    FE_FORCE_INLINE constexpr T AlignDown(T x, const TAlignmentType alignment)
    {
        return x & ~(static_cast<T>(alignment) - 1);
    }


    //! @brief Align down a pointer.
    //!
    //! @param x         Value to align.
    //! @param alignment Alignment to use.
    template<class T>
    FE_FORCE_INLINE T* AlignDownPtr(T* x, const size_t alignment)
    {
        return reinterpret_cast<T*>(AlignDown(reinterpret_cast<uintptr_t>(x), alignment));
    }


    //! @brief Align down an integer.
    //!
    //! @param x           Value to align.
    //! @tparam TAlignment Alignment to use.
    template<uint32_t TAlignment, class T>
    FE_FORCE_INLINE constexpr T AlignDown(const T x)
    {
        return x & ~(static_cast<T>(TAlignment) - 1);
    }


    //! @brief Check if an integer is aligned.
    //!
    //! @param x         Value to check.
    //! @param alignment Alignment to use.
    template<class T, class TAlignmentType = T>
    FE_FORCE_INLINE constexpr bool IsAligned(T x, const TAlignmentType alignment)
    {
        return (x & (static_cast<T>(alignment) - 1)) == 0;
    }


    //! @brief Check if a pointer is aligned.
    //!
    //! @param x         Value to check.
    //! @param alignment Alignment to use.
    template<class T>
    FE_FORCE_INLINE bool IsAlignedPtr(T* x, const size_t alignment)
    {
        return IsAligned(reinterpret_cast<uintptr_t>(x), alignment);
    }


    //! @brief Check if an integer is aligned.
    //!
    //! @param x           Value to check.
    //! @tparam TAlignment Alignment to use.
    template<uint32_t TAlignment, class T>
    FE_FORCE_INLINE constexpr bool IsAligned(const T x)
    {
        return (x & (static_cast<T>(TAlignment) - 1)) == 0;
    }


    //! @brief Create a bitmask.
    //!
    //! @param bitCount  The number of ones in the created mask.
    //! @param leftShift The number of zeros to the right of the created mask.
    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr T MakeMask(const T bitCount, const T leftShift)
    {
        const T allOnes = std::numeric_limits<T>::max();
        const T mask = static_cast<T>(-(bitCount != 0)) & (allOnes >> (sizeof(T) * 8 - bitCount));
        return mask << leftShift;
    }
} // namespace FE

namespace FE::Bit
{
    //! @brief Set a bit specified by bitIndex to one and return the modified value.
    template<class T>
    FE_FORCE_INLINE std::enable_if_t<std::is_unsigned_v<T>, T> Set(const T value, const T bitIndex)
    {
        return value | (T(1u) << bitIndex);
    }


    //! @brief Reset a bit specified by bitIndex to zero and return the modified value.
    template<class T>
    FE_FORCE_INLINE std::enable_if_t<std::is_unsigned_v<T>, T> Clear(const T value, const T bitIndex)
    {
        return value & ~(T(1u) << bitIndex);
    }


    //! @brief Count the number of trailing zeros in the given value.
    FE_FORCE_INLINE int32_t CountTrailingZeros(const uint32_t value)
    {
#if FE_COMPILER_MSVC
        unsigned long result = 0;
        if (_BitScanForward(&result, value))
            return result;
        return 32;
#else
        return __builtin_ctz(value);
#endif
    }


    //! @brief Count the number of trailing zeros in the given value.
    FE_FORCE_INLINE int32_t CountTrailingZeros(const uint64_t value)
    {
#if FE_COMPILER_MSVC
        unsigned long result = 0;
        if (_BitScanForward64(&result, value))
            return result;
        return 64;
#else
        return __builtin_ctzll(value);
#endif
    }


    //! @brief Count the number of leading zeros in the given value.
    FE_FORCE_INLINE int32_t CountLeadingZeros(const uint32_t value)
    {
#if FE_COMPILER_MSVC
        unsigned long result = 0;
        if (_BitScanReverse(&result, value))
            return 31 - result;
        return 32;
#else
        return __builtin_clz(value);
#endif
    }


    //! @brief Count the number of leading zeros in the given value.
    FE_FORCE_INLINE int32_t CountLeadingZeros(const uint64_t value)
    {
#if FE_COMPILER_MSVC
        unsigned long result = 0;
        if (_BitScanReverse64(&result, value))
            return 63 - result;
        return 64;
#else
        return __builtin_clzll(value);
#endif
    }


    //! @brief Count the number of set bits in the given value.
    FE_FORCE_INLINE uint32_t PopCount(const uint32_t value)
    {
#if FE_COMPILER_MSVC
        return __popcnt(value);
#else
        return __builtin_popcount(value);
#endif
    }


    //! @brief Count the number of set bits in the given value.
    FE_FORCE_INLINE uint32_t PopCount(const uint64_t value)
    {
#if FE_COMPILER_MSVC
        return static_cast<uint32_t>(__popcnt64(value));
#else
        return __builtin_popcountll(value);
#endif
    }


    //! @brief Search for the first set bit in the given value and store its index in result.
    //!
    //! @return true if a bit was found, false if the value is zero.
    FE_FORCE_INLINE bool ScanForward(uint32_t& result, const uint32_t value)
    {
#if FE_COMPILER_MSVC
        return _BitScanForward(reinterpret_cast<unsigned long*>(&result), value);
#else
        if (value == 0)
            return false;

        result = __builtin_ctz(value);
        return true;
#endif
    }


    //! @brief Search for the first set bit in the given value and store its index in result.
    //!
    //! @return true if a bit was found, false if the value is zero.
    FE_FORCE_INLINE bool ScanForward64(uint32_t& result, const uint64_t value)
    {
#if FE_COMPILER_MSVC
        return _BitScanForward64(reinterpret_cast<unsigned long*>(&result), value);
#else
        if (value == 0)
            return false;

        result = __builtin_ctzll(value);
        return true;
#endif
    }


    //! @brief Search for the last set bit in the given value and store its index in result.
    //!
    //! @return true if a bit was found, false if the value is zero.
    FE_FORCE_INLINE bool ScanReverse(uint32_t& result, const uint32_t value)
    {
#if FE_COMPILER_MSVC
        return _BitScanReverse(reinterpret_cast<unsigned long*>(&result), value);
#else
        if (value == 0)
            return false;

        result = 31 - __builtin_clz(value);
        return true;
#endif
    }


    //! @brief Search for the last set bit in the given value and store its index in result.
    //!
    //! @return true if a bit was found, false if the value is zero.
    FE_FORCE_INLINE bool ScanReverse64(uint32_t& result, const uint64_t value)
    {
#if FE_COMPILER_MSVC
        return _BitScanReverse64(reinterpret_cast<unsigned long*>(&result), value);
#else
        if (value == 0)
            return false;

        result = 63 - __builtin_clzll(value);
        return true;
#endif
    }


    //! @brief Traverse an unsigned integer and call a functor for each set bit.
    //!
    //! @param word    The unsigned integer to traverse.
    //! @param functor The functor to call for each set bit.
    template<class TFunctor>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Traverse(uint32_t word, TFunctor functor)
    {
        uint32_t currentIndex;
        while (ScanForward(currentIndex, word))
        {
            functor(currentIndex);
            word &= ~(1 << currentIndex);
        }
    }


    //! @brief Traverse an unsigned integer and call a functor for each set bit.
    //!
    //! @param word    The unsigned integer to traverse.
    //! @param functor The functor to call for each set bit.
    template<class TFunctor>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Traverse(uint64_t word, TFunctor functor)
    {
        uint32_t currentIndex;
        while (ScanForward64(currentIndex, word))
        {
            functor(currentIndex);
            word &= ~(UINT64_C(1) << currentIndex);
        }
    }


    //! @brief Check if all bits from `test` are set in the `source`.
    template<class TFlag>
    FE_FORCE_INLINE bool constexpr AllSet(const TFlag source, const TFlag test)
    {
        return (festd::to_underlying(source) & festd::to_underlying(test)) == festd::to_underlying(test);
    }


    //! @brief Check if any bit from `test` is set in the `source`.
    template<class TFlag>
    FE_FORCE_INLINE bool constexpr AnySet(const TFlag source, const TFlag test)
    {
        return (festd::to_underlying(source) & festd::to_underlying(test)) != static_cast<std::underlying_type_t<TFlag>>(0);
    }
} // namespace FE::Bit


namespace FE::Math
{
    //
    // The CompileTime namespace contains some functions that should only be used in
    // compile-time calculations. Using these in run-time may be inefficient.
    // The purpose is to emulate some non-constexpr intrinsics.
    //
    // TODO: this can be solved easily using c++20 is_constant_evaluated.
    //

    namespace CompileTime
    {
        //! @brief Multiply two 64-bit values with carry at compile time.
        FE_FORCE_INLINE constexpr uint64_t Multiply128(const uint64_t x, const uint64_t y, uint64_t* carry)
        {
            const uint64_t x0 = static_cast<uint32_t>(x), x1 = x >> 32;
            const uint64_t y0 = static_cast<uint32_t>(y), y1 = y >> 32;
            const uint64_t p11 = x1 * y1, p01 = x0 * y1;
            const uint64_t p10 = x1 * y0, p00 = x0 * y0;
            const uint64_t middle = p10 + (p00 >> 32) + static_cast<uint32_t>(p01);
            *carry = p11 + (middle >> 32) + (p01 >> 32);
            return (middle << 32) | static_cast<uint32_t>(p00);
        }
    } // namespace CompileTime


    FE_FORCE_INLINE float Sin(const float x)
    {
        return sinf(x);
    }


    FE_FORCE_INLINE float Cos(const float x)
    {
        return cosf(x);
    }


    FE_FORCE_INLINE float Tan(const float x)
    {
        return tanf(x);
    }


    FE_FORCE_INLINE float Asin(const float x)
    {
        return asinf(x);
    }


    FE_FORCE_INLINE float Acos(const float x)
    {
        return acosf(x);
    }


    FE_FORCE_INLINE float Sqrt(const float x)
    {
        return sqrtf(x);
    }


    FE_FORCE_INLINE float Abs(const float x)
    {
        return abs(x);
    }


    FE_FORCE_INLINE int32_t Abs(const int32_t x)
    {
        return abs(x);
    }


    FE_FORCE_INLINE int64_t Abs(const int64_t x)
    {
        return abs(x);
    }


    FE_FORCE_INLINE float Floor(const float x)
    {
        return floorf(x);
    }


    FE_FORCE_INLINE float Ceil(const float x)
    {
        return ceilf(x);
    }


    //! @brief Round a floating point number to the nearest integer.
    //!
    //! @param x The floating point number to round.
    //!
    //! @note This function behaves differently from std::round, it uses `floor(x + 0.5f)`.
    FE_FORCE_INLINE float Round(const float x)
    {
        return floorf(x + 0.5f);
    }


    FE_FORCE_INLINE bool FE_VECTORCALL EqualEstimate(const float lhs, const float rhs, const float epsilon = Constants::kEpsilon)
    {
        return abs(lhs - rhs) < epsilon;
    }


    template<class T>
    FE_FORCE_INLINE constexpr T Max(const T lhs, const T rhs)
    {
        return lhs > rhs ? lhs : rhs;
    }


    template<class T>
    FE_FORCE_INLINE constexpr T Min(const T lhs, const T rhs)
    {
        return lhs < rhs ? lhs : rhs;
    }


    template<class T>
    FE_FORCE_INLINE constexpr T Clamp(const T value, const T min, const T max)
    {
        return Max(min, Min(value, max));
    }


    FE_FORCE_INLINE constexpr float Saturate(const float value)
    {
        return Clamp(value, 0.0f, 1.0f);
    }


    FE_FORCE_INLINE constexpr uint32_t MakeFourCC(const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d)
    {
        return a | (b << 8u) | (c << 16u) | (d << 24u);
    }


    FE_FORCE_INLINE constexpr bool IsPowerOfTwo(const uint32_t x)
    {
        return x != 0 && (x & (x - 1)) == 0;
    }


    //! @brief Calculate the smallest power of two that is greater than or equal to `x`.
    FE_FORCE_INLINE uint32_t CeilPowerOfTwo(const uint32_t x)
    {
        const uint64_t x64 = 2 * static_cast<uint64_t>(x) - 1;
        uint32_t result;
        Bit::ScanReverse64(result, x64);
        return static_cast<uint32_t>(1 << result);
    }


    //! @brief Calculate the largest power of two that is less than or equal to `x`.
    FE_FORCE_INLINE uint32_t FloorPowerOfTwo(const uint32_t x)
    {
        return 1 << (31 - Bit::CountLeadingZeros(x));
    }


    template<class T1, class T2>
    FE_FORCE_INLINE auto CeilDivide(const T1 x, const T2 y)
        -> std::enable_if_t<std::is_unsigned_v<T1> && std::is_integral_v<T2>, decltype(x / y)>
    {
        return (x + y - 1) / y;
    }


    FE_FORCE_INLINE uint32_t FloorLog2(const uint32_t x)
    {
        uint32_t result;
        if (Bit::ScanReverse(result, x))
            return result;

        return Constants::kMaxU32;
    }


    //! @brief Represents a swizzle.
    //!
    //! @note The enum does not contain all the possible combinations as it would be too large.
    //!       Use MakeSwizzle() to create a swizzle that is not in this enum.
    enum class Swizzle : uint32_t
    {
        kXXXX = 0x00,
        kYYYY = 0x55,
        kZZZZ = 0xaa,
        kWWWW = 0xff,

        kXYZW = 0xe4,
    };


    FE_FORCE_INLINE constexpr Swizzle MakeSwizzle(const uint32_t x, const uint32_t y, const uint32_t z, const uint32_t w)
    {
        return static_cast<Swizzle>(x | (y << 2) | (z << 4) | (w << 6));
    }
} // namespace FE::Math


//! @brief Define bitwise operations on `enum`.
//!
//! The macro defines bitwise or, and, xor operators.
#define FE_ENUM_OPERATORS(Name)                                                                                                  \
    inline constexpr Name operator|(const Name a, const Name b)                                                                  \
    {                                                                                                                            \
        return Name(((std::underlying_type_t<Name>)a) | ((std::underlying_type_t<Name>)b));                                      \
    }                                                                                                                            \
    inline constexpr Name& operator|=(Name& a, const Name b)                                                                     \
    {                                                                                                                            \
        return a = a | b;                                                                                                        \
    }                                                                                                                            \
    inline constexpr Name operator&(const Name a, const Name b)                                                                  \
    {                                                                                                                            \
        return Name(((std::underlying_type_t<Name>)a) & ((std::underlying_type_t<Name>)b));                                      \
    }                                                                                                                            \
    inline constexpr Name& operator&=(Name& a, const Name b)                                                                     \
    {                                                                                                                            \
        return a = a & b;                                                                                                        \
    }                                                                                                                            \
    inline constexpr Name operator^(const Name a, const Name b)                                                                  \
    {                                                                                                                            \
        return Name(((std::underlying_type_t<Name>)a) ^ ((std::underlying_type_t<Name>)b));                                      \
    }                                                                                                                            \
    inline constexpr Name& operator^=(Name& a, const Name b)                                                                     \
    {                                                                                                                            \
        return a = a ^ b;                                                                                                        \
    }
