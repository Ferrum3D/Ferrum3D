#pragma once
#include <FeCore/Memory/Memory.h>
#include <HAL/Common/BaseTypes.h>
#include <HAL/Device.h>

namespace FE::Graphics::HAL
{
    class DeviceObject : public Memory::RefCountedObjectBase
    {
        friend class Device;

        DeviceObject(const DeviceObject&) = delete;
        DeviceObject& operator=(const DeviceObject&) = delete;

        DeviceObject(DeviceObject&&) = delete;
        DeviceObject& operator=(DeviceObject&&) = delete;

        inline void DoDispose()
        {
            std::pmr::memory_resource* pAllocator = m_pAllocator;
            const size_t allocationSize = m_AllocationSize;
            this->~DeviceObject();
            pAllocator->deallocate(this, allocationSize);
        }

    protected:
        Device* m_pDevice = nullptr;

        DeviceObject() = default;

        inline void DoRelease() override
        {
            m_pDevice->QueueObjectDispose(this);
        }

    public:
        FE_RTTI_Class(DeviceObject, "52579F06-74CD-4151-8099-4D4283E8B6B0");

        virtual ~DeviceObject() = default;

        [[nodiscard]] inline Device* GetDevice() const
        {
            return m_pDevice;
        }
    };
} // namespace FE::Graphics::HAL
