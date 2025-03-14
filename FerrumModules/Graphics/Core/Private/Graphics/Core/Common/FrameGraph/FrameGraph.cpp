#include <FeCore/DI/Activator.h>
#include <Graphics/Core/Common/FrameGraph/FrameGraph.h>
#include <Graphics/Core/Common/FrameGraph/FrameGraphResourcePool.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Viewport.h>

namespace FE::Graphics::Common
{
    FrameGraph::FrameGraph(Core::Device* device, FrameGraphResourcePool* resourcePool)
        : m_passes(&m_linearAllocator)
        , m_resources(&m_linearAllocator)
    {
        m_device = device;
        m_resourcePool = resourcePool;
    }


    FrameGraph::PassDataBase& FrameGraph::GetPassData(const uint32_t passIndex)
    {
        return m_passes[passIndex];
    }


    uint32_t FrameGraph::AddPassInternal(const uint32_t producerIndex, const Env::Name name)
    {
        PassData& passData = m_passes.push_back();
        passData.m_name = name;
        passData.m_type = Core::PassType::kGraphics;
        passData.m_producerIndex = producerIndex;
        return m_passes.size() - 1;
    }


    void FrameGraph::RegisterViewport(Core::Viewport* viewport)
    {
        m_viewport = viewport;
    }


    Core::Viewport* FrameGraph::GetViewport()
    {
        return m_viewport.Get();
    }


    Core::ImageHandle FrameGraph::GetRenderTarget() const
    {
        return m_currentRenderTargetHandle;
    }


    Core::ImageHandle FrameGraph::GetDepthStencil() const
    {
        return m_currentDepthStencilHandle;
    }


    void FrameGraph::AddPassProducer(Core::PassProducer* passProducer)
    {
        m_passProducers.push_back(passProducer);
    }


    void FrameGraph::Execute()
    {
        FE_PROFILER_ZONE();

        m_currentRenderTargetHandle = ImportImage(m_viewport->GetCurrentColorTarget());
        m_currentDepthStencilHandle = ImportImage(m_viewport->GetDepthTarget());

        for (const auto& passProducer : m_passProducers)
        {
            Core::FrameGraphBuilder builder{ this, m_passProducers.size() - 1 };
            passProducer->Setup(*this, builder, m_blackboard);
        }

        PrepareExecute();
        FE_Assert(m_currentContext);

        for (const PassData& passData : m_passes)
        {
            passData.m_execute(passData.m_executeCallbackData, m_currentContext.Get());
            FinishPassExecute(passData);
        }

        FinishExecute();
        FE_Assert(m_currentContext == nullptr);

        m_currentRenderTargetHandle = Core::ImageHandle::kInvalid;
        m_currentDepthStencilHandle = Core::ImageHandle::kInvalid;
    }


    Core::Image* FrameGraph::GetImage(const Core::ImageHandle image) const
    {
        const ResourceData& resourceData = m_resources[image.m_desc.m_resourceIndex];
        FE_AssertDebug(resourceData.m_resourceType == Core::ResourceType::kImage);
        return static_cast<Core::Image*>(resourceData.m_resource.Get());
    }


    Core::Buffer* FrameGraph::GetBuffer(const Core::BufferHandle buffer) const
    {
        const ResourceData& resourceData = m_resources[buffer.m_desc.m_resourceIndex];
        FE_AssertDebug(resourceData.m_resourceType == Core::ResourceType::kBuffer);
        return static_cast<Core::Buffer*>(resourceData.m_resource.Get());
    }


    Core::ImageHandle FrameGraph::ImportImage(Core::Image* image)
    {
        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, image->GetDesc() };
        resourceData.m_resource = image;
        resourceData.m_name = image->GetName();
        resourceData.m_isImported = true;
        resourceData.m_creatorPassIndex = kInvalidIndex;
        m_resources.push_back(resourceData);
        return Core::ImageHandle::Create(resourceIndex, 0);
    }


    Core::BufferHandle FrameGraph::ImportBuffer(Core::Buffer* buffer)
    {
        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, buffer->GetDesc() };
        resourceData.m_resource = buffer;
        resourceData.m_name = buffer->GetName();
        resourceData.m_isImported = true;
        resourceData.m_creatorPassIndex = kInvalidIndex;
        m_resources.push_back(resourceData);
        return Core::BufferHandle::Create(resourceIndex, 0);
    }


    void FrameGraph::ResourceData::AddAccess(ResourceAccess* access)
    {
        if (m_accessesListHead == nullptr)
        {
            m_accessesListHead = access;
            m_accessesListTail = access;
        }
        else
        {
            m_accessesListTail->m_next = access;
            m_accessesListTail = access;
        }
    }


    Core::BufferHandle FrameGraph::CreateBuffer(const uint32_t passIndex, const Env::Name name, const Core::BufferDesc& desc)
    {
        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, desc };
        resourceData.m_name = name;
        resourceData.m_creatorPassIndex = passIndex;
        m_resources.push_back(resourceData);
        return Core::BufferHandle::Create(resourceIndex, 0);
    }


    Core::ImageHandle FrameGraph::CreateImage(const uint32_t passIndex, const Env::Name name, const Core::ImageDesc& desc)
    {
        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, desc };
        resourceData.m_name = name;
        resourceData.m_creatorPassIndex = passIndex;
        m_resources.push_back(resourceData);
        return Core::ImageHandle::Create(resourceIndex, 0);
    }


    //
    // - Version 0 is for the resources which have not been accessed in any way yet.
    // - Version 1 is for the resources that are accessed for the first type. In this case, reading is only allowed for
    //   imported resources as it makes no sense to read from a resource that has just been created.
    //


    uint32_t FrameGraph::ReadResource(const uint32_t passIndex, const uint32_t resourceIndex, const uint32_t flags)
    {
        ResourceData& resourceData = m_resources[resourceIndex];

        const uint32_t resourceVersion = resourceData.GetLastVersion() + 1;
        if (resourceVersion == 1)
            FE_Assert(resourceData.m_isImported, "Reading from a resource that has just been created");

        ResourceAccess* resourceAccess = Memory::New<ResourceAccess>(&m_linearAllocator);
        resourceAccess->m_passIndex = passIndex;
        resourceAccess->m_resourceIndex = resourceIndex;
        resourceAccess->m_version = resourceVersion;
        resourceAccess->m_isWriteAccess = false;
        resourceAccess->m_flags = flags;

        resourceData.AddAccess(resourceAccess);
        return resourceVersion;
    }


    uint32_t FrameGraph::WriteResource(const uint32_t passIndex, const uint32_t resourceIndex, const uint32_t flags)
    {
        ResourceData& resourceData = m_resources[resourceIndex];

        const uint32_t resourceVersion = resourceData.GetLastVersion() + 1;

        ResourceAccess* resourceAccess = Memory::New<ResourceAccess>(&m_linearAllocator);
        resourceAccess->m_passIndex = passIndex;
        resourceAccess->m_resourceIndex = resourceIndex;
        resourceAccess->m_version = resourceVersion;
        resourceAccess->m_isWriteAccess = true;
        resourceAccess->m_flags = flags;

        resourceData.AddAccess(resourceAccess);
        return resourceVersion;
    }
} // namespace FE::Graphics::Common
