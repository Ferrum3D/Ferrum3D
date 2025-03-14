#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/Common/FrameGraph/FrameGraphResourcePool.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Common
{
    struct FrameGraph : public Core::FrameGraph
    {
        FE_RTTI_Class(FrameGraph, "39F873DC-8F3D-4821-BA66-92FCD380B69A");

        void RegisterViewport(Core::Viewport* viewport) override;
        Core::Viewport* GetViewport() override;

        Core::ImageHandle GetRenderTarget() const override;
        Core::ImageHandle GetDepthStencil() const override;

        void AddPassProducer(Core::PassProducer* passProducer) final;
        void Execute() final;

        Core::Image* GetImage(Core::ImageHandle image) const final;
        Core::Buffer* GetBuffer(Core::BufferHandle buffer) const final;

        Core::ImageHandle ImportImage(Core::Image* image) override;
        Core::BufferHandle ImportBuffer(Core::Buffer* buffer) override;

    protected:
        friend Core::FrameGraphBuilder;
        friend Core::FrameGraphPassBuilder;

        struct PassData final : public PassDataBase
        {
            uint32_t m_producerIndex = kInvalidIndex;
            Env::Name m_name;
            Core::PassType m_type;
        };

        struct ResourceAccess final
        {
            ResourceAccess* m_next = nullptr;
            uint32_t m_passIndex = kInvalidIndex;
            uint32_t m_resourceIndex : Core::Internal::kFrameGraphResourceIndexBits;
            uint32_t m_version : Core::Internal::kFrameGraphResourceVersionBits;
            uint32_t m_isWriteAccess : 1;
            uint32_t m_flags : 5;
        };

        struct ResourceData final
        {
            union
            {
                Core::BufferDesc m_bufferDesc;
                Core::ImageDesc m_imageDesc;
            };

            Rc<Core::Resource> m_resource;
            Env::Name m_name;
            uint32_t m_resourceIndex : Core::Internal::kFrameGraphResourceIndexBits;
            Core::ResourceType m_resourceType : 2;
            uint32_t m_isImported : 1;
            uint32_t m_creatorPassIndex = kInvalidIndex;
            ResourceAccess* m_accessesListHead = nullptr;
            ResourceAccess* m_accessesListTail = nullptr;

            ResourceData(const uint32_t resourceIndex, const Core::BufferDesc& desc)
                : m_bufferDesc(desc)
                , m_resourceIndex(resourceIndex)
                , m_resourceType(Core::ResourceType::kBuffer)
                , m_isImported(false)
            {
            }

            ResourceData(const uint32_t resourceIndex, const Core::ImageDesc& desc)
                : m_imageDesc(desc)
                , m_resourceIndex(resourceIndex)
                , m_resourceType(Core::ResourceType::kImage)
                , m_isImported(false)
            {
            }

            void AddAccess(ResourceAccess* access);

            [[nodiscard]] uint32_t GetLastVersion() const
            {
                return m_accessesListTail ? m_accessesListTail->m_version : 0;
            }
        };

        FrameGraph(Core::Device* device, FrameGraphResourcePool* resourcePool);

        virtual void PrepareExecute() = 0;
        virtual void FinishExecute() = 0;

        virtual void FinishPassExecute(const PassData& pass) = 0;

        PassDataBase& GetPassData(uint32_t passIndex) final;
        uint32_t AddPassInternal(uint32_t producerIndex, Env::Name name) final;

        Core::BufferHandle CreateBuffer(uint32_t passIndex, Env::Name name, const Core::BufferDesc& desc) final;
        Core::ImageHandle CreateImage(uint32_t passIndex, Env::Name name, const Core::ImageDesc& desc) final;

        uint32_t ReadResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) final;
        uint32_t WriteResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) final;

        SegmentedVector<PassData> m_passes;
        SegmentedVector<ResourceData> m_resources;
        FrameGraphResourcePool* m_resourcePool = nullptr;

        Rc<Core::Viewport> m_viewport;
        Core::ImageHandle m_currentRenderTargetHandle;
        Core::ImageHandle m_currentDepthStencilHandle;

        Rc<Core::FrameGraphContext> m_currentContext;
    };
} // namespace FE::Graphics::Common
