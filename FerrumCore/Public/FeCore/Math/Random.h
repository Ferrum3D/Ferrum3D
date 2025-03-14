#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float MapUniformUIntToFloat(const uint32_t x)
        {
            const uint32_t one = 0x3f800000;
            const uint32_t two = 0x3fffffff;
            return festd::bit_cast<float>((x | one) & two) - 1.0f;
        }
    } // namespace Math


    struct Random_Xoroshiro128Plus final
    {
        uint64_t m_state[2];

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Random_Xoroshiro128Plus()
        {
            Reset();
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Random_Xoroshiro128Plus(const uint64_t s0, const uint64_t s1)
        {
            Reset(s0, s1);
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Reset()
        {
            m_state[0] = 0x2d107c83320d5263;
            m_state[1] = 0x84599e555c837f00;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Reset(const uint64_t s0, const uint64_t s1)
        {
            m_state[0] = s0;
            m_state[1] = s1;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint64_t RandUInt64()
        {
            const uint64_t s0 = m_state[0];
            uint64_t s1 = m_state[1];
            const uint64_t result = s0 + s1;

            s1 ^= s0;
            m_state[0] = Rotate(s0, 24) ^ s1 ^ (s1 << 16);
            m_state[1] = Rotate(s1, 37);

            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float RandFloat()
        {
            return Math::MapUniformUIntToFloat(RandUInt64() & Constants::kMaxU32);
        }

    private:
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static uint64_t Rotate(const uint64_t x, const int32_t k)
        {
            return (x << k) | (x >> (64 - k));
        }
    };

    using DefaultRandom = Random_Xoroshiro128Plus;
} // namespace FE
