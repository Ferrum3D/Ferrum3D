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

        Core::RenderTargetHandle GetMainColorTarget() const override;
        Core::RenderTargetHandle GetMainDepthStencilTarget() const override;

        void AddPassProducer(Core::PassProducer* passProducer) final;
        void Execute() final;

        Core::RenderTarget* GetRenderTarget(Core::RenderTargetHandle image) const final;
        Core::Buffer* GetBuffer(Core::BufferHandle buffer) const final;

        Core::ImageDesc GetResourceDesc(Core::RenderTargetHandle image) const override;
        Core::BufferDesc GetResourceDesc(Core::BufferHandle buffer) const override;

        Env::Name GetResourceName(Core::RenderTargetHandle image) const override;
        Env::Name GetResourceName(Core::BufferHandle buffer) const override;

        Core::RenderTargetHandle ImportRenderTarget(Core::RenderTarget* image, Core::ImageAccessType access) override;
        Core::BufferHandle ImportBuffer(Core::Buffer* buffer, Core::BufferAccessType access) override;

    protected:
        friend Core::FrameGraphBuilder;
        friend Core::FrameGraphPassBuilder;

        struct ResourceAccess final
        {
            ResourceAccess* m_next = nullptr;
            uint32_t m_passIndex = kInvalidIndex;
            uint32_t m_resourceIndex : Core::Internal::kFrameGraphResourceIndexBits;
            uint32_t m_version : Core::Internal::kFrameGraphResourceVersionBits;
            uint32_t m_isWriteAccess : 1;
            uint32_t m_flags : 5;
        };

        struct PassData final : public PassDataBase
        {
            uint32_t m_refCount = 0;
            uint32_t m_producerIndex = kInvalidIndex;
            Env::Name m_name;
            Core::PassType m_type;
            ResourceAccess* m_accessesListHead = nullptr;
            ResourceAccess* m_accessesListTail = nullptr;

            void AddAccess(ResourceAccess* access);
        };

        struct ResourceData final
        {
            union
            {
                Core::BufferDesc m_bufferDesc;
                Core::ImageDesc m_imageDesc;
            };

            Rc<Core::Resource> m_resource;
            uint32_t m_refCount = 0;
            Env::Name m_name;
            uint32_t m_resourceIndex : Core::Internal::kFrameGraphResourceIndexBits;
            Core::ResourceType m_resourceType : 2;
            uint32_t m_isImported : 1;
            uint32_t m_accessState = 0;
            uint32_t m_creatorPassIndex = kInvalidIndex;
            uint32_t m_lastUserPassIndex = kInvalidIndex;

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
                , m_resourceType(Core::ResourceType::kRenderTarget)
                , m_isImported(false)
            {
            }
        };

        FrameGraph(Core::Device* device, FrameGraphResourcePool* resourcePool);

        void Compile();

        virtual void PrepareSetup() = 0;
        virtual void PrepareExecute() = 0;
        virtual void FinishExecute() = 0;

        virtual void PreparePassExecute(uint32_t passIndex) = 0;

        PassDataBase& GetPassData(uint32_t passIndex) final;
        uint32_t AddPassInternal(uint32_t producerIndex, Env::Name name) final;

        Core::BufferHandle CreateBuffer(uint32_t passIndex, Env::Name name, const Core::BufferDesc& desc) final;
        Core::RenderTargetHandle CreateImage(uint32_t passIndex, Env::Name name, const Core::ImageDesc& desc) final;

        uint32_t ReadResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) final;
        uint32_t WriteResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) final;

        uint32_t GetResourceVersion(uint32_t resourceIndex) const;

        SegmentedVector<PassData> m_passes;
        SegmentedVector<ResourceData> m_resources;
        FrameGraphResourcePool* m_resourcePool = nullptr;

        Rc<Core::Viewport> m_viewport;
        Core::RenderTargetHandle m_currentRenderTargetHandle;
        Core::RenderTargetHandle m_currentDepthStencilHandle;

        Rc<Core::FrameGraphContext> m_currentContext;
    };
} // namespace FE::Graphics::Common
