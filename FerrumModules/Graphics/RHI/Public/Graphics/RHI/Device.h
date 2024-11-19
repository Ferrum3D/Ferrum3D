#pragma once
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Parallel/SpinLock.h>
#include <Graphics/RHI/Common/BaseTypes.h>

namespace FE::Graphics::RHI
{
    struct CommandQueue;
    struct DeviceService;

    struct Device
        : public Memory::RefCountedObjectBase
        , public EventBus<FrameEvents>::Handler
    {
        FE_RTTI_Class(Device, "23D426E6-3322-4CB2-9800-DEBA7C3DEAC0");

        ~Device() override
        {
            FE_CORE_ASSERT(m_disposeQueue.empty(), "Device implementation didn't call DisposePending()");
        }

        virtual void WaitIdle() = 0;

        [[nodiscard]] virtual Rc<CommandQueue> GetCommandQueue(HardwareQueueKindFlags cmdQueueClass) = 0;

    protected:
        Logger* m_logger;

        Device(Logger* pLogger)
            : m_logger(pLogger)
        {
        }

        void DisposePending();

    private:
        friend struct DeviceObject;
        friend struct DeviceService;
        friend struct Resource;

        struct PendingDisposer final
        {
            RHI::DeviceObject* m_object = nullptr;
            uint32_t m_framesLeft = 0;
        };

        SpinLock m_resourceListLock;
        SpinLock m_disposeQueueLock;
        SpinLock m_serviceListLock;
        festd::intrusive_list<> m_resourceList;
        festd::vector<PendingDisposer> m_disposeQueue;
        festd::vector<DeviceService*> m_serviceList;

        void QueueObjectDispose(DeviceObject* pObject);
        void OnFrameEnd(const FrameEventArgs& args) override;
    };
} // namespace FE::Graphics::RHI
