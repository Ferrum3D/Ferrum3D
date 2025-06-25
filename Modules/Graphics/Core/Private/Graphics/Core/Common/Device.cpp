#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Common/Device.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Common
{
    Device::Device(Logger* pLogger)
        : m_logger(pLogger)
    {
#if FE_DEVELOPMENT
#    define FE_FORMAT_FUNC(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index) Core::Format::k##name,

        constexpr Core::Format formats[] = { FE_EXPAND_FORMATS(FE_FORMAT_FUNC) };

        festd::vector<uint32_t> formatIndices;
        formatIndices.reserve(festd::size(formats));

        for (const Core::Format format : formats)
        {
            const Core::FormatInfo formatInfo{ format };
            for (const uint32_t index : formatIndices)
            {
                FE_Assert(formatInfo.m_formatIndex != index);
            }

            formatIndices.push_back(formatInfo.m_formatIndex);
        }
#endif
    }


    void Device::DisposePending()
    {
        FE_PROFILER_ZONE();

        for (const PendingDisposer& disposer : m_disposeQueue)
        {
            // Don't use a range-based for loop here, since more objects can be added while we are iterating
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
        disposer.m_framesLeft = 10;
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

        Memory::FiberTempAllocator temp;

        // Copy to avoid deadlocks
        festd::pmr::vector<Core::DeviceObject*> objectsToDispose{ &temp };
        {
            std::lock_guard lock{ m_disposeQueueLock };
            objectsToDispose = GetObjectsToDispose(&temp, false);
        }

        for (Core::DeviceObject* object : objectsToDispose)
            object->DoDispose();
    }


    void Device::ForceReleasePendingDisposers()
    {
        FE_PROFILER_ZONE();

        for (;;)
        {
            // TODO: this can be called from outside of the fibers, so we cannot use the temp allocator
            // Memory::FiberTempAllocator temp;

            // Copy to avoid deadlocks
            festd::pmr::vector<Core::DeviceObject*> objectsToDispose;
            {
                std::lock_guard lock{ m_disposeQueueLock };
                objectsToDispose = GetObjectsToDispose(std::pmr::get_default_resource(), true);
            }

            if (objectsToDispose.empty())
                break;

            for (Core::DeviceObject* object : objectsToDispose)
                object->DoDispose();
        }
    }


    festd::pmr::vector<Core::DeviceObject*> Device::GetObjectsToDispose(std::pmr::memory_resource* allocator, const bool force)
    {
        festd::pmr::vector<Core::DeviceObject*> objectsToDispose{ allocator };
        objectsToDispose.reserve(m_disposeQueue.size());
        for (PendingDisposer& disposer : m_disposeQueue)
        {
            if (force || --disposer.m_framesLeft == 0)
                objectsToDispose.push_back(disposer.m_object);
        }

        if (force)
        {
            m_disposeQueue.clear();
        }
        else
        {
            m_disposeQueue.erase(eastl::remove_if(m_disposeQueue.begin(),
                                                  m_disposeQueue.end(),
                                                  [](const PendingDisposer& disposer) {
                                                      return disposer.m_framesLeft == 0;
                                                  }),
                                 m_disposeQueue.end());
        }

        return objectsToDispose;
    }
} // namespace FE::Graphics::Common
