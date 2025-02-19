#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/RHI/Base/BaseTypes.h>
#include <Graphics/RHI/Device.h>

namespace FE::Graphics::RHI
{
    struct DeviceObject : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(DeviceObject, "52579F06-74CD-4151-8099-4D4283E8B6B0");

        [[nodiscard]] Device* GetDevice() const
        {
            return m_device;
        }

    private:
        friend struct Device;

        void DoDispose()
        {
            std::pmr::memory_resource* pAllocator = m_allocator;
            const size_t allocationSize = m_allocationSize;
            this->~DeviceObject();
            pAllocator->deallocate(this, allocationSize);
        }

    protected:
        Device* m_device = nullptr;

        DeviceObject() = default;

        void DoRelease() override
        {
            // TODO: suspicious... probably unsafe when calling from a different module.
            m_device->QueueObjectDispose(this);
        }
    };
} // namespace FE::Graphics::RHI
