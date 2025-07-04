#pragma once
#include <FeCore/Logging/Trace.h>
#include <FeCore/Threading/SpinLock.h>
#include <Graphics/Core/Device.h>
#include <festd/vector.h>

namespace FE::Graphics::Common
{
    struct Device : public Core::Device
    {
    protected:
        Logger* m_logger;

        explicit Device(Logger* pLogger);

        void DisposePending();

    private:
        struct PendingDisposer final
        {
            Core::DeviceObject* m_object = nullptr;
            uint32_t m_framesLeft = 0;
        };

        TracyLockable(Threading::SpinLock, m_resourceListLock);
        TracyLockable(Threading::SpinLock, m_disposeQueueLock);

        festd::intrusive_list<> m_resourceList;
        festd::vector<PendingDisposer> m_disposeQueue;

        uint32_t m_currentResourceId = 0;
        festd::vector<uint32_t> m_resourceIdFreeList;

        void QueueObjectDispose(Core::DeviceObject* object) override;
        uint32_t RegisterResource(Core::Resource* resource) override;
        void UnregisterResource(uint32_t id, Core::Resource* resource) override;
        void EndFrame() override;
    };
} // namespace FE::Graphics::Common
