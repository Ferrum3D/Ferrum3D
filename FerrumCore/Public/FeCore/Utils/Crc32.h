#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE
{
    struct Crc32 final
    {
        static uint32_t Compute(const void* data, size_t byteSize, uint32_t seed = 0);

        uint32_t Update(const void* data, const size_t byteSize)
        {
            m_current = Compute(data, byteSize, m_current);
            return m_current;
        }

        void Reset()
        {
            m_current = 0;
        }

        uint32_t m_current = 0;
    };
} // namespace FE
