#pragma once
#include <FeCore/Base/CompilerTraits.h>

namespace FE::Bit
{
    FE_FORCE_INLINE int32_t CountTrailingZeros(uint32_t value)
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


    FE_FORCE_INLINE int32_t CountLeadingZeros(uint32_t value)
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


    FE_FORCE_INLINE bool ScanForward(uint32_t& result, uint32_t value)
    {
#if FE_COMPILER_MSVC
        return _BitScanForward((unsigned long*)&result, value);
#else
        if (value == 0)
            return false;

        result = __builtin_ctz(value);
        return true;
#endif
    }


    FE_FORCE_INLINE bool ScanReverse(uint32_t& result, uint32_t value)
    {
#if FE_COMPILER_MSVC
        return _BitScanReverse((unsigned long*)&result, value);
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
        inline constexpr float Epsilon = 0.0001f;
    } // namespace Constants


    template<class T>
    inline constexpr T ToRadians(T degrees)
    {
        return degrees * Constants::PI / static_cast<T>(180);
    }


    template<class T>
    inline constexpr T ToDegrees(T radians)
    {
        return radians * static_cast<T>(180) / Constants::PI;
    }


    inline constexpr uint32_t MakeFourCC(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        return a | (b << 8u) | (c << 16u) | (d << 24u);
    }


    inline constexpr bool IsPowerOfTwo(uint32_t x)
    {
        return x != 0 && (x & (x - 1)) == 0;
    }


    inline constexpr uint32_t NextPowerOfTwo(uint32_t x)
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
    inline auto CeilDivide(T1 x, T2 y) -> std::enable_if_t<std::is_unsigned_v<T1> && std::is_integral_v<T2>, decltype(x / y)>
    {
        return (x + y - 1) / y;
    }


    inline uint32_t FloorLog2(uint32_t x)
    {
        uint32_t result;
        if (Bit::ScanReverse(result, x))
            return result;

        return UINT32_MAX;
    }
} // namespace FE::Math
