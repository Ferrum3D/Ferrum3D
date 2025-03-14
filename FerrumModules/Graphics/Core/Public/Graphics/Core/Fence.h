#pragma once
#include <Graphics/Core/Base/BaseTypes.h>
#include <Graphics/Core/DeviceObject.h>

namespace FE::Graphics::Core
{
    struct Fence : public DeviceObject
    {
        FE_RTTI_Class(Fence, "D815152F-A41F-45C8-81AB-F921F19E8AA3");

        virtual ResultCode Init(uint64_t initialValue = 0) = 0;

        virtual uint64_t GetCompletedValue() = 0;
        virtual void Wait(uint64_t value) = 0;
        virtual void Signal(uint64_t value) = 0;
    };


    struct FenceSyncPoint final
    {
        Rc<Fence> m_fence;
        uint64_t m_value = 0;

        [[nodiscard]] bool IsReady() const
        {
            return m_fence->GetCompletedValue() >= m_value;
        }

        void Wait() const
        {
            m_fence->Wait(m_value);
        }

        void Signal() const
        {
            m_fence->Signal(m_value);
        }
    };
} // namespace FE::Graphics::Core
