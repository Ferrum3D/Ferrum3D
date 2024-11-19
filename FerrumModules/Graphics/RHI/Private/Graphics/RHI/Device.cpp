#include <Graphics/RHI/Device.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/DeviceService.h>
#include <Graphics/RHI/Resource.h>

namespace FE::Graphics::RHI
{
    void Device::DisposePending()
    {
        for (uint32_t i = 0; i < m_disposeQueue.size(); ++i)
        {
            // Don't use a range-based for loop here, since more objects can be added while we are iterating
            PendingDisposer& disposer = m_disposeQueue[i];
            disposer.m_object->DoDispose();
            m_logger->LogInfo("Deleted object at {}", reinterpret_cast<uintptr_t>(disposer.m_object));
        }
        for (festd::intrusive_list_node& resourceNode : m_resourceList)
        {
            m_logger->LogError("Resource leak: {}", static_cast<Resource&>(resourceNode).GetName());
            FE_DebugBreak();
        }
        for (DeviceService* service : m_serviceList)
        {
            service->Shutdown();
            service->Release();
        }

        m_disposeQueue.clear();
        m_resourceList.clear();
        m_serviceList.clear();
    }


    void Device::QueueObjectDispose(DeviceObject* pObject)
    {
        std::lock_guard lock{ m_disposeQueueLock };
        PendingDisposer& disposer = m_disposeQueue.push_back();
        disposer.m_object = pObject;
        disposer.m_framesLeft = 3;
    }


    void Device::OnFrameEnd(const FrameEventArgs&)
    {
        std::lock_guard lock{ m_disposeQueueLock };
        for (uint32_t i = 0; i < m_disposeQueue.size();)
        {
            m_logger->LogInfo("Trying to delete object at {}, frames left: {}...",
                              reinterpret_cast<uintptr_t>(m_disposeQueue[i].m_object),
                              m_disposeQueue[i].m_framesLeft);
            if (--m_disposeQueue[i].m_framesLeft > 0)
            {
                ++i;
                continue;
            }

            m_disposeQueue[i].m_object->DoDispose();
            m_logger->LogInfo("Deleted object at {}", reinterpret_cast<uintptr_t>(m_disposeQueue[i].m_object));
            m_disposeQueue.erase_unsorted(m_disposeQueue.begin() + i);
        }
    }
} // namespace FE::Graphics::RHI
