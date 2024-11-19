#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/RHI/Common/BaseTypes.h>
#include <Graphics/RHI/Device.h>

namespace FE::Graphics::RHI
{
    struct DeviceObject : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(DeviceObject, "52579F06-74CD-4151-8099-4D4283E8B6B0");

        virtual ~DeviceObject() = default;

        [[nodiscard]] Device* GetDevice() const
        {
            return m_device;
        }

    private:
        friend struct Device;

        DeviceObject(const DeviceObject&) = delete;
        DeviceObject& operator=(const DeviceObject&) = delete;

        DeviceObject(DeviceObject&&) = delete;
        DeviceObject& operator=(DeviceObject&&) = delete;

        void DoDispose()
        {
            std::pmr::memory_resource* pAllocator = m_pAllocator;
            const size_t allocationSize = m_AllocationSize;
            this->~DeviceObject();
            pAllocator->deallocate(this, allocationSize);
        }

    protected:
        Device* m_device = nullptr;

        DeviceObject() = default;

        void DoRelease() override
        {
            m_device->QueueObjectDispose(this);
        }
    };
} // namespace FE::Graphics::RHI
