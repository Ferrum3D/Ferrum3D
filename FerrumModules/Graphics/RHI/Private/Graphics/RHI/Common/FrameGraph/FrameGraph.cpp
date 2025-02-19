#include <FeCore/DI/Activator.h>
#include <Graphics/RHI/Common/FrameGraph/FrameGraph.h>
#include <Graphics/RHI/Common/FrameGraph/FrameGraphResourcePool.h>

namespace FE::Graphics::Common
{
    FrameGraph::FrameGraph()
        : m_passes(&m_linearAllocator)
        , m_resources(&m_linearAllocator)
    {
        m_resourcePool = DI::DefaultNew<FrameGraphResourcePool>().value();
    }


    FrameGraph::PassDataBase& FrameGraph::GetPassData(const uint32_t passIndex)
    {
        return m_passes[passIndex];
    }


    uint32_t FrameGraph::AddPassInternal(const uint32_t producerIndex, const Env::Name name,
                                         const RHI::GraphicsPipelineDesc& pipelineDesc)
    {
        PassData& passData = m_passes.push_back();
        passData.m_name = name;
        passData.m_type = RHI::PassType::kGraphics;
        passData.m_producerIndex = producerIndex;
        passData.m_graphicsPipelineDesc = pipelineDesc;
        return m_passes.size() - 1;
    }


    FrameGraph::~FrameGraph()
    {
        Memory::Delete(&m_linearAllocator, m_resourcePool, sizeof(FrameGraphResourcePool));
    }


    void FrameGraph::AddPassProducer(RHI::PassProducer* passProducer)
    {
        m_passProducers.push_back(passProducer);

        struct FrameGraphBuilder : public RHI::FrameGraphBuilder
        {
            FrameGraphBuilder(FrameGraph* graph, const uint32_t passProducerIndex)
                : RHI::FrameGraphBuilder(graph, passProducerIndex)
            {
            }
        };

        FrameGraphBuilder builder{ this, m_passProducers.size() - 1 };
        passProducer->Setup(builder, m_blackboard);
    }


    void FrameGraph::Execute()
    {
        for (const PassData& passData : m_passes)
        {
        }
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


    RHI::BufferHandle FrameGraph::CreateBuffer(const uint32_t passIndex, const Env::Name name, const RHI::BufferDesc& desc)
    {
        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, desc };
        resourceData.m_name = name;
        resourceData.m_creatorPassIndex = passIndex;
        m_resources.push_back(resourceData);
        return RHI::BufferHandle::Create(resourceIndex, 0);
    }


    RHI::ImageHandle FrameGraph::CreateImage(const uint32_t passIndex, const Env::Name name, const RHI::ImageDesc& desc)
    {
        const uint32_t resourceIndex = m_resources.size();
        ResourceData resourceData{ resourceIndex, desc };
        resourceData.m_name = name;
        resourceData.m_creatorPassIndex = passIndex;
        m_resources.push_back(resourceData);
        return RHI::ImageHandle::Create(resourceIndex, 0);
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
            FE_AssertMsg(resourceData.m_isImported, "Reading from a resource that has just been created");

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
