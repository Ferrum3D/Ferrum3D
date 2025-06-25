#include <FeCore/DI/Activator.h>
#include <FeCore/Memory/FiberTempAllocator.h>
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


    Core::RenderTargetHandle FrameGraph::GetMainColorTarget() const
    {
        return m_currentRenderTargetHandle;
    }


    Core::RenderTargetHandle FrameGraph::GetMainDepthStencilTarget() const
    {
        return m_currentDepthStencilHandle;
    }


    void FrameGraph::AddPassProducer(Core::PassProducer* passProducer)
    {
        m_passProducers.push_back(passProducer);
    }


    void FrameGraph::Compile()
    {
        FE_PROFILER_ZONE();

        Memory::FiberTempAllocator temp;

        for (PassData& pass : m_passes)
        {
            const ResourceAccess* access = pass.m_accessesListHead;
            while (access)
            {
                if (access->m_isWriteAccess)
                {
                    ++pass.m_refCount;
                }
                else
                {
                    auto& resource = m_resources[access->m_resourceIndex];
                    ++resource.m_refCount;
                }

                access = access->m_next;
            }
        }

        SegmentedVector<const ResourceData*> resourceStack{ &temp };

        for (ResourceData& resource : m_resources)
        {
            resource.m_refCount = 1000; // TODO: hack to prevent culling until it's fixed
            if (resource.m_refCount == 0)
                resourceStack.push_back(&resource);
        }

        while (!resourceStack.empty())
        {
            const ResourceData* resource = resourceStack.back();
            resourceStack.pop_back();

            if (resource->m_isImported)
                continue;

            auto& creatorPass = m_passes[resource->m_creatorPassIndex];
            --creatorPass.m_refCount;

            const ResourceAccess* access = creatorPass.m_accessesListHead;
            while (access)
            {
                if (!access->m_isWriteAccess)
                {
                    auto& readResource = m_resources[access->m_resourceIndex];
                    --readResource.m_refCount;
                    if (readResource.m_refCount == 0)
                        resourceStack.push_back(&readResource);
                }

                access = access->m_next;
            }
        }
    }


    void FrameGraph::Execute()
    {
        FE_PROFILER_ZONE();

        PrepareSetup();

        if (m_viewport)
        {
            Core::RenderTarget* colorTarget = m_viewport->GetCurrentColorTarget();
            const Core::ImageDesc colorTargetDesc = colorTarget->GetDesc();
            const Core::ImageDesc depthTargetDesc =
                Core::ImageDesc::Img2D(colorTargetDesc.m_width, colorTargetDesc.m_height, Core::Format::kD32_SFLOAT_S8_UINT);

            ResourceData depthTargetData{ 0, depthTargetDesc };
            depthTargetData.m_name = "MainDepthTarget";
            depthTargetData.m_creatorPassIndex = kInvalidIndex;
            depthTargetData.m_resource = m_resourcePool->CreateRenderTarget(depthTargetData.m_name, depthTargetDesc);
            depthTargetData.m_isImported = true;
            depthTargetData.m_refCount = 1;
            m_resources.push_back(depthTargetData);

            m_currentDepthStencilHandle = Core::RenderTargetHandle::Create(0, 0, 0);
            m_currentRenderTargetHandle = ImportRenderTarget(colorTarget, Core::ImageAccessType::kUndefined);
        }

        for (uint32_t passProducerIndex = 0; passProducerIndex < m_passProducers.size(); ++passProducerIndex)
        {
            Core::PassProducer* passProducer = m_passProducers[passProducerIndex];
            Core::FrameGraphBuilder builder{ this, passProducerIndex };
            passProducer->Setup(*this, builder, m_blackboard);
        }

        Compile();

        PrepareExecute();
        FE_Assert(m_currentContext);

        for (auto& resource : m_resources)
        {
            if (resource.m_isImported)
            {
                FE_Assert(resource.m_resource);
                continue;
            }

            if (resource.m_refCount > 0)
            {
                switch (resource.m_resourceType)
                {
                default:
                case Core::ResourceType::kTexture:
                case Core::ResourceType::kUnknown:
                    FE_DebugBreak();
                    [[fallthrough]];

                case Core::ResourceType::kBuffer:
                    resource.m_resource = m_resourcePool->CreateBuffer(resource.m_name, resource.m_bufferDesc);
                    break;

                case Core::ResourceType::kRenderTarget:
                    resource.m_resource = m_resourcePool->CreateRenderTarget(resource.m_name, resource.m_imageDesc);
                    break;
                }
            }
        }

        for (uint32_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
        {
            const PassData& pass = m_passes[passIndex];
            if (pass.m_refCount > 0)
            {
                PreparePassExecute(passIndex);
                pass.m_execute(pass.m_executeCallbackData, m_currentContext.Get());
            }
        }

        FinishExecute();
        m_resourcePool->Reset();

        FE_Assert(m_currentContext == nullptr);

        m_currentRenderTargetHandle = Core::RenderTargetHandle::kInvalid;
        m_currentDepthStencilHandle = Core::RenderTargetHandle::kInvalid;
    }


    Core::RenderTarget* FrameGraph::GetRenderTarget(const Core::RenderTargetHandle image) const
    {
        const ResourceData& resourceData = m_resources[image.m_desc.m_resourceIndex];
        FE_AssertDebug(resourceData.m_resourceType == Core::ResourceType::kRenderTarget);
        return static_cast<Core::RenderTarget*>(resourceData.m_resource.Get());
    }


    Core::Buffer* FrameGraph::GetBuffer(const Core::BufferHandle buffer) const
    {
        const ResourceData& resourceData = m_resources[buffer.m_desc.m_resourceIndex];
        FE_AssertDebug(resourceData.m_resourceType == Core::ResourceType::kBuffer);
        return static_cast<Core::Buffer*>(resourceData.m_resource.Get());
    }


    Core::ImageDesc FrameGraph::GetResourceDesc(const Core::RenderTargetHandle image) const
    {
        const ResourceData& resourceData = m_resources[image.m_desc.m_resourceIndex];
        return resourceData.m_imageDesc;
    }


    Core::BufferDesc FrameGraph::GetResourceDesc(const Core::BufferHandle buffer) const
    {
        const ResourceData& resourceData = m_resources[buffer.m_desc.m_resourceIndex];
        return resourceData.m_bufferDesc;
    }


    Env::Name FrameGraph::GetResourceName(const Core::RenderTargetHandle image) const
    {
        const ResourceData& resourceData = m_resources[image.m_desc.m_resourceIndex];
        return resourceData.m_name;
    }


    Env::Name FrameGraph::GetResourceName(const Core::BufferHandle buffer) const
    {
        const ResourceData& resourceData = m_resources[buffer.m_desc.m_resourceIndex];
        return resourceData.m_name;
    }


    Core::RenderTargetHandle FrameGraph::ImportRenderTarget(Core::RenderTarget* image, const Core::ImageAccessType access)
    {
        FE_Assert(image);

        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, image->GetDesc() };
        resourceData.m_resource = image;
        resourceData.m_name = image->GetName();
        resourceData.m_isImported = true;
        resourceData.m_creatorPassIndex = kInvalidIndex;
        resourceData.m_accessState = festd::to_underlying(access);
        m_resources.push_back(resourceData);
        return Core::RenderTargetHandle::Create(resourceIndex, 0, 0);
    }


    Core::BufferHandle FrameGraph::ImportBuffer(Core::Buffer* buffer, const Core::BufferAccessType access)
    {
        FE_Assert(buffer);

        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, buffer->GetDesc() };
        resourceData.m_resource = buffer;
        resourceData.m_name = buffer->GetName();
        resourceData.m_isImported = true;
        resourceData.m_creatorPassIndex = kInvalidIndex;
        resourceData.m_accessState = festd::to_underlying(access);
        m_resources.push_back(resourceData);
        return Core::BufferHandle::Create(resourceIndex, 0, 0);
    }


    void FrameGraph::PassData::AddAccess(ResourceAccess* access)
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
        FE_AssertDebug(passIndex < m_passes.size());

        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, desc };
        resourceData.m_name = name;
        resourceData.m_creatorPassIndex = passIndex;
        m_resources.push_back(resourceData);
        return Core::BufferHandle::Create(resourceIndex, 0, 0);
    }


    Core::RenderTargetHandle FrameGraph::CreateImage(const uint32_t passIndex, const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_AssertDebug(passIndex < m_passes.size());

        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, desc };
        resourceData.m_name = name;
        resourceData.m_creatorPassIndex = passIndex;
        m_resources.push_back(resourceData);
        return Core::RenderTargetHandle::Create(resourceIndex, 0, 0);
    }


    //
    // - Version 0 is for the resources which have not been accessed in any way yet.
    // - Version 1 is for the resources that are accessed for the first time. In this case, reading is only allowed for
    //   imported resources as it makes no sense to read from a resource that has just been created.
    //


    uint32_t FrameGraph::ReadResource(const uint32_t passIndex, const uint32_t resourceIndex, const uint32_t flags)
    {
        FE_AssertDebug(passIndex < m_passes.size());
        FE_Assert(resourceIndex < m_resources.size());

        ResourceData& resourceData = m_resources[resourceIndex];

        const uint32_t resourceVersion = GetResourceVersion(resourceIndex) + 1;
        if (resourceVersion == 1)
            FE_Assert(resourceData.m_isImported, "Reading from a resource that has just been created");

        ResourceAccess* resourceAccess = Memory::New<ResourceAccess>(&m_linearAllocator);
        resourceAccess->m_passIndex = passIndex;
        resourceAccess->m_resourceIndex = resourceIndex;
        resourceAccess->m_version = resourceVersion;
        resourceAccess->m_isWriteAccess = false;
        resourceAccess->m_flags = flags;

        auto& resource = m_resources[resourceIndex];
        resource.m_lastUserPassIndex = passIndex;

        auto& pass = m_passes[passIndex];
        pass.AddAccess(resourceAccess);
        return resourceVersion;
    }


    uint32_t FrameGraph::WriteResource(const uint32_t passIndex, const uint32_t resourceIndex, const uint32_t flags)
    {
        FE_AssertDebug(passIndex < m_passes.size());
        FE_Assert(resourceIndex < m_resources.size());

        const uint32_t resourceVersion = GetResourceVersion(resourceIndex) + 1;

        ResourceAccess* resourceAccess = Memory::New<ResourceAccess>(&m_linearAllocator);
        resourceAccess->m_passIndex = passIndex;
        resourceAccess->m_resourceIndex = resourceIndex;
        resourceAccess->m_version = resourceVersion;
        resourceAccess->m_isWriteAccess = true;
        resourceAccess->m_flags = flags;

        auto& resource = m_resources[resourceIndex];
        resource.m_lastUserPassIndex = passIndex;

        auto& pass = m_passes[passIndex];
        pass.AddAccess(resourceAccess);
        return resourceVersion;
    }


    uint32_t FrameGraph::GetResourceVersion(const uint32_t resourceIndex) const
    {
        const auto& resource = m_resources[resourceIndex];
        if (resource.m_lastUserPassIndex == kInvalidIndex)
            return 0;

        const auto& pass = m_passes[resource.m_lastUserPassIndex];

        const ResourceAccess* access = pass.m_accessesListHead;
        while (access)
        {
            if (access->m_resourceIndex == resourceIndex)
                return access->m_version;

            access = access->m_next;
        }

        FE_DebugBreak();
        return kInvalidIndex;
    }
} // namespace FE::Graphics::Common
