#include <HAL/Device.h>
#include <HAL/DeviceObject.h>

namespace FE::Graphics::HAL
{
    Device::~Device()
    {
        for (auto& disposer : m_DisposeQueue)
        {
            disposer.pObject->DoDispose();
            m_pLogger->LogMessage("Deleted object at {}", disposer.pObject);
        }
    }


    void Device::QueueObjectDispose(DeviceObject* pObject)
    {
        std::lock_guard lock{ m_DisposeQueueLock };
        PendingDisposer& disposer = m_DisposeQueue.push_back();
        disposer.pObject = pObject;
        disposer.FramesLeft = 3;
    }


    void Device::OnFrameEnd(const FrameEventArgs&)
    {
        std::lock_guard lock{ m_DisposeQueueLock };
        for (uint32_t i = 0; i < m_DisposeQueue.size();)
        {
            m_pLogger->LogMessage(
                "Trying to delete object at {}, frames left: {}...", m_DisposeQueue[i].pObject, m_DisposeQueue[i].FramesLeft);
            if (--m_DisposeQueue[i].FramesLeft > 0)
            {
                ++i;
                continue;
            }

            m_DisposeQueue[i].pObject->DoDispose();
            m_pLogger->LogMessage("Deleted object at {}", m_DisposeQueue[i].pObject);
            m_DisposeQueue.erase_unsorted(m_DisposeQueue.begin() + i);
        }
    }
} // namespace FE::Graphics::HAL
