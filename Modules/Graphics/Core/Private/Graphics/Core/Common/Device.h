#pragma once
#include <Core/Logging/Trace.h>
#include <Core/Threading/SpinLock.h>
#include <Graphics/Core/Device.h>
#include <festd/vector.h>

namespace FE::Graphics::Common
{
    struct Device : public Core::Device
    {
        FE_RTTI("830776B9-20AC-4D55-B7DC-685EA6D20CC2");

        const festd::intrusive_list<Core::Resource>& GetResources() const
        {
            return m_resourceList;
        }

    protected:
        explicit Device();

    private:
        TracyLockable(Threading::SpinLock, m_resourceListLock);

        festd::intrusive_list<Core::Resource> m_resourceList;

        uint32_t m_currentResourceId = 0;
        festd::vector<uint32_t> m_resourceIdFreeList;

        uint32_t RegisterResource(Core::Resource* resource) override;
        void UnregisterResource(uint32_t id, Core::Resource* resource) override;
    };
} // namespace FE::Graphics::Common
