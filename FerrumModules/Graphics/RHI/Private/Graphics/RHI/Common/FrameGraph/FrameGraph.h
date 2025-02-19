#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/RHI/Common/FrameGraph/FrameGraphResourcePool.h>
#include <Graphics/RHI/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Common
{
    struct FrameGraph : public RHI::FrameGraph
    {
        ~FrameGraph() override;

        void AddPassProducer(RHI::PassProducer* passProducer) final;
        void Execute() final;

        RHI::Image* GetImage(const RHI::ImageHandle image) const final
        {
            const ResourceData& resourceData = m_resources[image.m_desc.m_resourceIndex];
            FE_Assert(resourceData.m_resourceType == RHI::ResourceType::kImage);
            return fe_assert_cast<RHI::Image*>(resourceData.m_resource.Get());
        }

        RHI::Buffer* GetBuffer(const RHI::BufferHandle buffer) const final
        {
            const ResourceData& resourceData = m_resources[buffer.m_desc.m_resourceIndex];
            FE_Assert(resourceData.m_resourceType == RHI::ResourceType::kBuffer);
            return fe_assert_cast<RHI::Buffer*>(resourceData.m_resource.Get());
        }

    protected:
        friend RHI::FrameGraphBuilder;
        friend RHI::FrameGraphPassBuilder;

        struct PassData final : public PassDataBase
        {
            RHI::GraphicsPipelineDesc m_graphicsPipelineDesc;
            uint32_t m_producerIndex = kInvalidIndex;
            Env::Name m_name;
            RHI::PassType m_type;
        };

        struct ResourceAccess final
        {
            ResourceAccess* m_next = nullptr;
            uint32_t m_passIndex = kInvalidIndex;
            uint32_t m_resourceIndex : RHI::Internal::kFrameGraphResourceIndexBits;
            uint32_t m_version : RHI::Internal::kFrameGraphResourceVersionBits;
            uint32_t m_isWriteAccess : 1;
            uint32_t m_flags : 5;
        };

        struct ResourceData final
        {
            union
            {
                RHI::BufferDesc m_bufferDesc;
                RHI::ImageDesc m_imageDesc;
            };

            Rc<RHI::Resource> m_resource;
            Env::Name m_name;
            uint32_t m_resourceIndex : RHI::Internal::kFrameGraphResourceIndexBits;
            RHI::ResourceType m_resourceType : 2;
            uint32_t m_isImported : 1;
            uint32_t m_creatorPassIndex = kInvalidIndex;
            ResourceAccess* m_accessesListHead = nullptr;
            ResourceAccess* m_accessesListTail = nullptr;

            ResourceData(const uint32_t resourceIndex, const RHI::BufferDesc& desc)
                : m_bufferDesc(desc)
                , m_resourceIndex(resourceIndex)
                , m_resourceType(RHI::ResourceType::kBuffer)
                , m_isImported(false)
            {
            }

            ResourceData(const uint32_t resourceIndex, const RHI::ImageDesc& desc)
                : m_imageDesc(desc)
                , m_resourceIndex(resourceIndex)
                , m_resourceType(RHI::ResourceType::kImage)
                , m_isImported(false)
            {
            }

            void AddAccess(ResourceAccess* access);

            [[nodiscard]] uint32_t GetLastVersion() const
            {
                return m_accessesListTail ? m_accessesListTail->m_version : 0;
            }
        };

        FrameGraph();

        PassDataBase& GetPassData(uint32_t passIndex) final;
        uint32_t AddPassInternal(uint32_t producerIndex, Env::Name name, const RHI::GraphicsPipelineDesc& pipelineDesc) final;

        RHI::BufferHandle CreateBuffer(uint32_t passIndex, Env::Name name, const RHI::BufferDesc& desc) final;
        RHI::ImageHandle CreateImage(uint32_t passIndex, Env::Name name, const RHI::ImageDesc& desc) final;

        uint32_t ReadResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) final;
        uint32_t WriteResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) final;

        SegmentedVector<PassData> m_passes;
        SegmentedVector<ResourceData> m_resources;
        FrameGraphResourcePool* m_resourcePool = nullptr;
    };
} // namespace FE::Graphics::Common
