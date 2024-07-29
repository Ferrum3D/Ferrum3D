#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    namespace Constants
    {
        inline constexpr float PI = 3.14159265358979323f;
        inline constexpr float Epsilon = 0.0001f;
    }

    FE_FORCE_INLINE constexpr float ToRadians(float degrees) noexcept
    {
        return degrees * Constants::PI / 180.f;
    }

    FE_FORCE_INLINE constexpr float ToDegrees(float radians) noexcept
    {
        return radians * 180.f / Constants::PI;
    }

    FE_FORCE_INLINE constexpr UInt32 NextPowerOfTwo(UInt32 v) noexcept
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

    FE_FORCE_INLINE UInt32 CountTrailingZeros(UInt32 value) noexcept
    {
        unsigned long tz = 0;
        return _BitScanForward(&tz, value) ? tz : 32;
    }

    FE_FORCE_INLINE UInt32 CountLeadingZeros(UInt32 value) noexcept
    {
        unsigned long lz = 0;
        return _BitScanReverse(&lz, value) ? 31 - lz : 32;
    }

#else

    FE_FORCE_INLINE UInt32 CountTrailingZeros(UInt32 value) noexcept
    {
        return __builtin_ctz(value);
    }

    FE_FORCE_INLINE UInt32 CountLeadingZeros(UInt32 value) noexcept
    {
        return __builtin_clz(value);
    }

#endif

} // namespace FE
