#pragma once
#include <Shaders/Database/Base.h>

namespace DB
{
    template<typename T, uint32_t TOffset>
    struct ElementHandle
    {
        BufferPointer m_ptr;

        void Setup(BufferPointer page, const uint32_t localRowIndex)
        {
            m_ptr.SetDeviceAddress(page.GetDeviceAddress() + TOffset + localRowIndex * sizeof(T));
        }

        T Get() FE_CONST
        {
            return m_ptr.Read<T>(0);
        }
    };
} // namespace DB
