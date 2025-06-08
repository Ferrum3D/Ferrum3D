#pragma once
#include <Graphics/Core/Device.h>

namespace FE::Graphics::Common
{
    struct Device;
}

namespace FE::Graphics::Core
{
    struct DeviceObject : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(DeviceObject, "52579F06-74CD-4151-8099-4D4283E8B6B0");

        [[nodiscard]] Device* GetDevice() const
        {
            return m_device;
        }

        void SetImmediateDestroyPolicy()
        {
            m_destroyedImmediately = true;
        }

    private:
        friend Common::Device;

        void DoDispose()
        {
            std::pmr::memory_resource* pAllocator = GetObjectAllocator();
            const size_t allocationSize = GetObjectAllocationSize();
            this->~DeviceObject();
            pAllocator->deallocate(this, allocationSize);
        }

    protected:
        Device* m_device = nullptr;
        bool m_destroyedImmediately = false;

        DeviceObject() = default;

        void DoRelease() override
        {
            if (m_destroyedImmediately)
            {
                DoDispose();
                return;
            }

            m_device->QueueObjectDispose(this);
        }
    };
} // namespace FE::Graphics::Core
