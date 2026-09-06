#pragma once
#include <Core/RTTI/RTTI.h>

namespace FE
{
    struct Crc32 final
    {
        FE_RTTI_Reflect("C7A8826D-2B98-4ABB-BB12-57036464C1DE");
        FE_RTTI_Serialize();

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
