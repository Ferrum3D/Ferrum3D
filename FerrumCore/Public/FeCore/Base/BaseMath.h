#pragma once
#include <FeCore/Base/CompilerTraits.h>

namespace FE
{
    //! @brief Align up an integer.
    //!
    //! @param x     - Value to align.
    //! @param align - Alignment to use.
    template<class T, class U = T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T AlignUp(T x, U align)
    {
        return static_cast<T>((x + (align - 1u)) & ~(align - 1u));
    }

    //! @brief Align up a pointer.
    //!
    //! @param x     - Value to align.
    //! @param align - Alignment to use.
    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T* AlignUpPtr(const T* x, size_t align)
    {
        return reinterpret_cast<T*>(AlignUp(reinterpret_cast<size_t>(x), align));
    }

    //! @brief Align up an integer.
    //!
    //! @param x  - Value to align.
    //! @tparam A - Alignment to use.
    template<uint32_t A, class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr T AlignUp(T x)
    {
        return (x + (A - 1)) & ~(A - 1);
    }

    //! @brief Align down an integer.
    //!
    //! @param x     - Value to align.
    //! @param align - Alignment to use.
    template<class T, class U = T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T AlignDown(T x, U align)
    {
        return x & ~(align - 1);
    }

    //! @brief Align down a pointer.
    //!
    //! @param x     - Value to align.
    //! @param align - Alignment to use.
    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr T* AlignDownPtr(const T* x, size_t align)
    {
        return reinterpret_cast<T*>(AlignDown(reinterpret_cast<size_t>(x), align));
    }

    //! @brief Align down an integer.
    //!
    //! @param x  - Value to align.
    //! @tparam A - Alignment to use.
    template<uint32_t A, class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr T AlignDown(T x)
    {
        return x & ~(A - 1);
    }

    //! @brief Create a bitmask.
    //!
    //! @param bitCount  - The number of ones in the created mask.
    //! @param leftShift - The number of zeros to the right of the created mask.
    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr T MakeMask(T bitCount, T leftShift)
    {
        auto typeBitCount = sizeof(T) * 8;
        auto mask = bitCount == typeBitCount ? static_cast<T>(-1) : ((1 << bitCount) - 1);
        return static_cast<T>(mask << leftShift);
    }


    template<class TTo, class TFrom>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE
        std::enable_if_t<std::is_default_constructible_v<TTo> && sizeof(TTo) == sizeof(TFrom), TTo>
        bit_cast(const TFrom& value)
    {
        TTo result;
        memcpy(&result, &value, sizeof(TTo));
        return result;
    }

    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr auto enum_cast(T value) -> std::underlying_type_t<T>
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }
} // namespace FE

namespace FE::Bit
{
    //! @brief Count the number of trailing zeros in the given value.
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE int32_t CountTrailingZeros(uint32_t value)
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


    //! @brief Count the number of leading zeros in the given value.
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE int32_t CountLeadingZeros(uint32_t value)
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


    //! @brief Search for the first set bit in the given value and store its index in result.
    //!
    //! @return true if a bit was found, false if the value is zero.
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool ScanForward(uint32_t& result, uint32_t value)
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


    //! @brief Search for the last set bit in the given value and store its index in result.
    //!
    //! @return true if a bit was found, false if the value is zero.
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool ScanReverse(uint32_t& result, uint32_t value)
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
        FE_FORCE_INLINE constexpr uint64_t Multiply128(uint64_t x, uint64_t y, uint64_t* carry)
        {
            const uint64_t x0 = static_cast<uint32_t>(x), x1 = x >> 32;
            const uint64_t y0 = static_cast<uint32_t>(y), y1 = y >> 32;
            const uint64_t p11 = x1 * y1, p01 = x0 * y1;
            const uint64_t p10 = x1 * y0, p00 = x0 * y0;
            const uint64_t middle = p10 + (p00 >> 32) + (uint32_t)p01;
            *carry = p11 + (middle >> 32) + (p01 >> 32);
            return (middle << 32) | (uint32_t)p00;
        }
    } // namespace CompileTime


    namespace Constants
    {
        inline constexpr float PI = 3.14159265358979323f;
        inline constexpr float Epsilon = 1e-6f;
    } // namespace Constants


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Sin(float x)
    {
        return sinf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Cos(float x)
    {
        return cosf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Tan(float x)
    {
        return tanf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Asin(float x)
    {
        return asinf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Acos(float x)
    {
        return acosf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Sqrt(float x)
    {
        return sqrtf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Abs(float x)
    {
        return abs(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE int32_t Abs(int32_t x)
    {
        return abs(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE int64_t Abs(int64_t x)
    {
        return abs(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Floor(float x)
    {
        return floorf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Ceil(float x)
    {
        return ceilf(x);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Round(float x)
    {
        return floorf(x + 0.5f);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(float lhs, float rhs,
                                                                           float epsilon = Constants::Epsilon)
    {
        return abs(lhs - rhs) < epsilon;
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T Max(T lhs, T rhs)
    {
        return lhs > rhs ? lhs : rhs;
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T Min(T lhs, T rhs)
    {
        return lhs < rhs ? lhs : rhs;
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T Clamp(T value, T min, T max)
    {
        return Max(min, Min(value, max));
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Saturate(float value)
    {
        return Clamp(value, 0.0f, 1.0f);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr uint32_t MakeFourCC(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        return a | (b << 8u) | (c << 16u) | (d << 24u);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr bool IsPowerOfTwo(uint32_t x)
    {
        return x != 0 && (x & (x - 1)) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE constexpr uint32_t NextPowerOfTwo(uint32_t x)
    {
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x++;
        return x;
    }


    template<class T1, class T2>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE auto CeilDivide(T1 x, T2 y)
        -> std::enable_if_t<std::is_unsigned_v<T1> && std::is_integral_v<T2>, decltype(x / y)>
    {
        return (x + y - 1) / y;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FloorLog2(uint32_t x)
    {
        uint32_t result;
        if (Bit::ScanReverse(result, x))
            return result;

        return UINT32_MAX;
    }


    //! @brief Represents a component of a vector.
    enum class Component : uint32_t
    {
        kX = 0,
        kY = 1,
        kZ = 2,
        kW = 3,

        kR = kX,
        kG = kY,
        kB = kZ,
        kA = kW,
    };


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


    FE_FORCE_INLINE constexpr Swizzle MakeSwizzle(Component x, Component y, Component z, Component w)
    {
        return static_cast<Swizzle>(enum_cast(x) | (enum_cast(y) << 2) | (enum_cast(z) << 4) | (enum_cast(w) << 6));
    }
} // namespace FE::Math
