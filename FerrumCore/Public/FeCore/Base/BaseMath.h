#pragma once
#include <FeCore/Base/CompilerTraits.h>

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


    inline constexpr int32_t CountTrailingZeros(uint32_t value)
    {
        return __builtin_ctz(value);
    }


    inline constexpr int32_t CountLeadingZeros(uint32_t value)
    {
        return __builtin_clz(value);
    }


    template<class T1, class T2>
    inline auto CeilDivide(T1 x, T2 y) -> std::enable_if_t<std::is_unsigned_v<T1> && std::is_integral_v<T2>, decltype(x / y)>
    {
        return (x + y - 1) / y;
    }


    inline constexpr int32_t FloorLog2(uint32_t x)
    {
        return sizeof(int32_t) * 8 - CountLeadingZeros(x) - 1;
    }

} // namespace FE::Math
