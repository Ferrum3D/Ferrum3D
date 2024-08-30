#pragma once
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Parallel/SpinLock.h>
#include <HAL/BindFlags.h>
#include <HAL/Common/BaseTypes.h>

namespace FE::Graphics::HAL
{
    class CommandQueue;
    class DeviceService;

    class Device
        : public Memory::RefCountedObjectBase
        , public EventBus<FrameEvents>::Handler
    {
        friend class DeviceObject;
        friend class DeviceService;
        friend class Resource;

        struct PendingDisposer final
        {
            HAL::DeviceObject* pObject = nullptr;
            uint32_t FramesLeft = 0;
        };

        SpinLock m_ResourceListLock;
        SpinLock m_DisposeQueueLock;
        SpinLock m_ServiceListLock;
        festd::intrusive_list<> m_ResourceList;
        festd::vector<PendingDisposer> m_DisposeQueue;
        festd::vector<DeviceService*> m_ServiceList;

        void QueueObjectDispose(DeviceObject* pObject);
        void OnFrameEnd(const FrameEventArgs& args) override;

    protected:
        Logger* m_pLogger;

        inline Device(Logger* pLogger)
            : m_pLogger(pLogger)
        {
        }

        void DisposePending();

    public:
        FE_RTTI_Class(Device, "23D426E6-3322-4CB2-9800-DEBA7C3DEAC0");

        inline ~Device() override
        {
            FE_CORE_ASSERT(m_DisposeQueue.empty(), "Device implementation didn't call DisposePending()");
        }

        virtual void WaitIdle() = 0;

        [[nodiscard]] virtual Rc<CommandQueue> GetCommandQueue(HardwareQueueKindFlags cmdQueueClass) = 0;
    };
} // namespace FE::Graphics::HAL
