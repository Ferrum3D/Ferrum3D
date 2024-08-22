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

        struct PendingDisposer final
        {
            HAL::DeviceObject* pObject = nullptr;
            uint32_t FramesLeft = 0;
        };

        SpinLock m_DisposeQueueLock;
        festd::vector<PendingDisposer> m_DisposeQueue;

        Debug::IConsoleLogger* m_pLogger;

        void QueueObjectDispose(DeviceObject* pObject);
        void OnFrameEnd(const FrameEventArgs& args) override;

    protected:
        inline Device(Debug::IConsoleLogger* pLogger)
            : m_pLogger(pLogger)
        {
        }

    public:
        FE_RTTI_Class(Device, "23D426E6-3322-4CB2-9800-DEBA7C3DEAC0");

        ~Device() override;

        virtual void WaitIdle() = 0;

        [[nodiscard]] virtual Rc<CommandQueue> GetCommandQueue(HardwareQueueKindFlags cmdQueueClass) = 0;
    };
} // namespace FE::Graphics::HAL
