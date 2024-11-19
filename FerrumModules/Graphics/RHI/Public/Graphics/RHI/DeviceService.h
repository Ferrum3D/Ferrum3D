#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/RHI/Device.h>

namespace FE::Graphics::RHI
{
    struct DeviceService
        : public Memory::RefCountedObjectBase
        , public festd::intrusive_list_node
    {
        virtual void Shutdown() {}

    protected:
        Device* m_device = nullptr;

        DeviceService(Device* pDevice)
            : m_device(pDevice)
        {
            std::lock_guard lk{ m_device->m_serviceListLock };
            m_device->m_serviceList.push_back(this);
            AddRef();
        }
    };
} // namespace FE::Graphics::RHI
