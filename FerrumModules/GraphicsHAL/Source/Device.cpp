#include <HAL/Device.h>
#include <HAL/DeviceObject.h>
#include <HAL/DeviceService.h>
#include <HAL/Resource.h>

namespace FE::Graphics::HAL
{
    void Device::DisposePending()
    {
        for (uint32_t i = 0; i < m_DisposeQueue.size(); ++i)
        {
            // Don't use a range-based for loop here, since more objects can be added while we are iterating
            PendingDisposer& disposer = m_DisposeQueue[i];
            disposer.pObject->DoDispose();
            m_pLogger->LogMessage("Deleted object at {}", disposer.pObject);
        }
        for (festd::intrusive_list_node& resourceNode : m_ResourceList)
        {
            m_pLogger->LogError("Resource leak: {}", static_cast<HAL::Resource&>(resourceNode).GetName());
            FE_DEBUGBREAK;
        }
        for (DeviceService* pService : m_ServiceList)
        {
            pService->Release();
        }

        m_DisposeQueue.clear();
        m_ResourceList.clear();
        m_ServiceList.clear();
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
