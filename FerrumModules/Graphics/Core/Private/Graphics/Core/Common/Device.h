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
        TracyLockable(Threading::SpinLock, m_serviceListLock);

        festd::intrusive_list<> m_resourceList;
        festd::vector<PendingDisposer> m_disposeQueue;

        void QueueObjectDispose(Core::DeviceObject* object) override;
        void RegisterResource(Core::Resource* resource) override;
        void EndFrame() override;
    };
} // namespace FE::Graphics::Common
