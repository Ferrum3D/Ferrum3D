#pragma once
#include <FeCore/Memory/Memory.h>
#include <HAL/Device.h>

namespace FE::Graphics::HAL
{
    class DeviceService
        : public Memory::RefCountedObjectBase
        , public festd::intrusive_list_node
    {
    protected:
        Device* m_pDevice = nullptr;

        inline DeviceService(Device* pDevice)
            : m_pDevice(pDevice)
        {
            std::lock_guard lk{ m_pDevice->m_ServiceListLock };
            m_pDevice->m_ServiceList.push_back(*this);
            AddRef();
        }

    public:
    };
} // namespace FE::Graphics::HAL
