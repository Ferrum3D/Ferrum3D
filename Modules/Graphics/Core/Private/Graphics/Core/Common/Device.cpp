#include <Graphics/Core/Common/Device.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Common
{
    Device::Device(Logger* pLogger)
        : m_logger(pLogger)
    {
#define FE_FORMAT_FUNC(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index) Core::Format::k##name,

        constexpr Core::Format formats[] = { FE_EXPAND_FORMATS(FE_FORMAT_FUNC) };

        festd::vector<uint32_t> formatIndices;
        formatIndices.reserve(festd::size(formats));

        for (const Core::Format format : formats)
        {
            const Core::FormatInfo formatInfo{ format };

            if (Build::IsDebug())
            {
                for (const uint32_t index : formatIndices)
                {
                    FE_Assert(formatInfo.m_formatIndex != index);
                }
            }

            formatIndices.push_back(formatInfo.m_formatIndex);
        }
    }


    void Device::DisposePending()
    {
        FE_PROFILER_ZONE();

        for (uint32_t i = 0; i < m_disposeQueue.size(); ++i)
        {
            // Don't use a range-based for loop here, since more objects can be added while we are iterating
            const PendingDisposer disposer = m_disposeQueue[i];
            disposer.m_object->DoDispose();
            m_logger->LogInfo("Deleted object at {}", reinterpret_cast<uintptr_t>(disposer.m_object));
        }
        for (festd::intrusive_list_node& resourceNode : m_resourceList)
        {
            m_logger->LogError("Resource leak: {}", static_cast<Core::Resource&>(resourceNode).GetName());
            FE_DebugBreak();
        }

        m_disposeQueue.clear();
        m_resourceList.clear();
    }


    void Device::QueueObjectDispose(Core::DeviceObject* object)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_disposeQueueLock };
        PendingDisposer& disposer = m_disposeQueue.push_back();
        disposer.m_object = object;
        disposer.m_framesLeft = 3;
    }


    uint32_t Device::RegisterResource(Core::Resource* resource)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_resourceListLock };
        m_resourceList.push_back(*resource);

        if (!m_resourceIdFreeList.empty())
        {
            const uint32_t id = m_resourceIdFreeList.back();
            m_resourceIdFreeList.pop_back();
            return id;
        }

        return m_currentResourceId++;
    }


    void Device::UnregisterResource(const uint32_t id, Core::Resource* resource)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_resourceListLock };
        festd::intrusive_list<>::remove(*resource);
        m_resourceIdFreeList.push_back(id);
    }


    void Device::EndFrame()
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_disposeQueueLock };
        for (uint32_t i = 0; i < m_disposeQueue.size();)
        {
            // m_logger->LogInfo("Trying to delete object at {}, frames left: {}...",
            //                   reinterpret_cast<uintptr_t>(m_disposeQueue[i].m_object),
            //                   m_disposeQueue[i].m_framesLeft);
            if (--m_disposeQueue[i].m_framesLeft > 0)
            {
                ++i;
                continue;
            }

            m_disposeQueue[i].m_object->DoDispose();
            // m_logger->LogInfo("Deleted object at {}", reinterpret_cast<uintptr_t>(m_disposeQueue[i].m_object));
            m_disposeQueue.erase_unsorted(m_disposeQueue.begin() + i);
        }
    }
} // namespace FE::Graphics::Common
