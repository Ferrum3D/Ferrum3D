#pragma once
#include <FeCore/Console/IConsoleLogger.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <FeCore/Parallel/SpinLock.h>
#include <HAL/BindFlags.h>
#include <HAL/Common/BaseTypes.h>

namespace FE::Graphics::HAL
{
    class CommandQueue;

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

        SpinLock m_DisposeQueueLock;
        SpinLock m_ResourceListLock;
        SpinLock m_ServiceListLock;
        festd::vector<PendingDisposer> m_DisposeQueue;
        festd::intrusive_list<> m_ResourceList;
        festd::intrusive_list<> m_ServiceList;

        Debug::IConsoleLogger* m_pLogger;

        void QueueObjectDispose(DeviceObject* pObject);
        void OnFrameEnd(const FrameEventArgs& args) override;

    protected:
        inline Device(Debug::IConsoleLogger* pLogger)
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
