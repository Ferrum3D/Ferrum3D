#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    namespace Constants
    {
        inline constexpr Float32 PI = 3.14159265358979323f;
        inline constexpr Float32 Epsilon = 0.0001f;
    }

    FE_FINLINE constexpr Float32 ToRadians(Float32 degrees) noexcept
    {
        return degrees * Constants::PI / 180.f;
    }

    FE_FINLINE constexpr Float32 ToDegrees(Float32 radians) noexcept
    {
        return radians * 180.f / Constants::PI;
    }

    FE_FINLINE constexpr UInt32 NextPowerOfTwo(UInt32 v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    inline constexpr char IntToHexChar(Int32 n)
    {
        return "0123456789ABCDEF"[n];
    }

#ifdef FE_COMPILER_MSVC

    FE_FINLINE UInt32 CountTrailingZeros(UInt32 value) noexcept
    {
        unsigned long tz = 0;
        return _BitScanForward(&tz, value) ? tz : 32;
    }

    FE_FINLINE UInt32 CountLeadingZeros(UInt32 value) noexcept
    {
        unsigned long lz = 0;
        return _BitScanReverse(&lz, value) ? 31 - lz : 32;
    }

#else

    FE_FINLINE UInt32 CountTrailingZeros(UInt32 value) noexcept
    {
        return __builtin_ctz(value);
    }

    FE_FINLINE UInt32 CountLeadingZeros(UInt32 value) noexcept
    {
        return __builtin_clz(value);
    }

#endif

} // namespace FE
