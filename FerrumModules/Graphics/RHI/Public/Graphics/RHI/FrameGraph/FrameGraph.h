#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/CommandList.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/FrameGraph/Blackboard.h>
#include <Graphics/RHI/GraphicsPipeline.h>
#include <Graphics/RHI/Image.h>

namespace FE::Graphics::RHI
{
    using FrameGraphPassFunction = void(CommandList* commandList);


    struct PassProducer : public Memory::RefCountedObjectBase
    {
        virtual void Setup(FrameGraphBuilder& builder, FrameGraphBlackboard& blackboard) = 0;
    };


    struct FrameGraph : public DeviceObject
    {
        ~FrameGraph() override;

        void AddPassProducer(PassProducer* passProducer);
        void Execute();

        Image* GetImage(const ImageHandle image) const
        {
            const ResourceData& resourceData = m_resources[image.m_desc.m_resourceIndex];
            FE_Assert(resourceData.m_resourceType == ResourceType::kImage);
            return fe_assert_cast<Image*>(resourceData.m_resource.Get());
        }

        Buffer* GetBuffer(const BufferHandle buffer) const
        {
            const ResourceData& resourceData = m_resources[buffer.m_desc.m_resourceIndex];
            FE_Assert(resourceData.m_resourceType == ResourceType::kBuffer);
            return fe_assert_cast<Buffer*>(resourceData.m_resource.Get());
        }

    protected:
        friend FrameGraphBuilder;
        friend FrameGraphPassBuilder;

        struct PassData final
        {
            void* m_executeCallbackData = nullptr;
            void (*m_execute)(void* callbackData, CommandList* commandList) = nullptr;
            uint32_t m_producerIndex = kInvalidIndex;
            Env::Name m_name;
            PassType m_type;
            GraphicsPipelineDesc m_graphicsPipelineDesc;

            ~PassData()
            {
                if (m_execute)
                    m_execute->~IPassFunction();
            }
        };

        struct ResourceAccess final
        {
            ResourceAccess* m_next = nullptr;
            uint32_t m_passIndex = kInvalidIndex;
            uint32_t m_resourceIndex : Internal::kFrameGraphResourceIndexBits;
            uint32_t m_version : Internal::kFrameGraphResourceVersionBits;
            uint32_t m_isWriteAccess : 1;
            uint32_t m_flags : 5;
        };

        struct ResourceData final
        {
            union
            {
                BufferDesc m_bufferDesc;
                ImageDesc m_imageDesc;
            };

            Rc<Resource> m_resource;
            Env::Name m_name;
            uint32_t m_resourceIndex : Internal::kFrameGraphResourceIndexBits;
            ResourceType m_resourceType : 2;
            uint32_t m_isImported : 1;
            uint32_t m_creatorPassIndex = kInvalidIndex;
            ResourceAccess* m_accessesListHead = nullptr;
            ResourceAccess* m_accessesListTail = nullptr;

            ResourceData(const uint32_t resourceIndex, const BufferDesc& desc)
                : m_bufferDesc(desc)
                , m_resourceIndex(resourceIndex)
                , m_resourceType(ResourceType::kBuffer)
                , m_isImported(false)
            {
            }

            ResourceData(const uint32_t resourceIndex, const ImageDesc& desc)
                : m_imageDesc(desc)
                , m_resourceIndex(resourceIndex)
                , m_resourceType(ResourceType::kImage)
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

        uint32_t AddPassInternal(uint32_t producerIndex, Env::Name name, const GraphicsPipelineDesc& pipelineDesc);

        BufferHandle CreateBuffer(uint32_t passIndex, Env::Name name, const BufferDesc& desc);
        ImageHandle CreateImage(uint32_t passIndex, Env::Name name, const ImageDesc& desc);

        uint32_t ReadResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags);
        uint32_t WriteResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags);

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
        SegmentedVector<Rc<PassProducer>> m_passProducers;
        SegmentedVector<PassData> m_passes;
        SegmentedVector<ResourceData> m_resources;
        FrameGraphResourcePool* m_resourcePool = nullptr;
    };


    struct [[nodiscard]] FrameGraphPassBuilder final
    {
        BufferHandle CreateBuffer(const Env::Name name, const BufferDesc& desc) const
        {
            return m_graph->CreateBuffer(m_passIndex, name, desc);
        }

        ImageHandle CreateImage(const Env::Name name, const ImageDesc& desc) const
        {
            return m_graph->CreateImage(m_passIndex, name, desc);
        }

        BufferHandle Read(const BufferHandle buffer, const BufferReadType readType) const
        {
            const uint32_t flags = festd::to_underlying(readType);
            const uint32_t newVersion = m_graph->ReadResource(m_passIndex, buffer.m_desc.m_resourceIndex, flags);
            return BufferHandle::Create(buffer.m_desc.m_resourceIndex, newVersion);
        }

        ImageHandle Read(const ImageHandle image, const ImageReadType readType) const
        {
            const uint32_t flags = festd::to_underlying(readType);
            const uint32_t newVersion = m_graph->ReadResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return ImageHandle::Create(image.m_desc.m_resourceIndex, newVersion);
        }

        BufferHandle Write(const BufferHandle buffer, const BufferWriteType writeType) const
        {
            const uint32_t flags = festd::to_underlying(writeType);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, buffer.m_desc.m_resourceIndex, flags);
            return BufferHandle::Create(buffer.m_desc.m_resourceIndex, newVersion);
        }

        ImageHandle Write(const ImageHandle image, const ImageWriteType writeType) const
        {
            const uint32_t flags = festd::to_underlying(writeType);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return ImageHandle::Create(image.m_desc.m_resourceIndex, newVersion);
        }

        template<class TFunction>
        void SetFunction(TFunction&& function) const
        {
            TFunction* f = Memory::New<TFunction>(&m_graph->m_linearAllocator, std::forward<TFunction>(function));

            auto& passData = m_graph->m_passes[m_passIndex];
            passData.m_executeCallbackData = f;
            passData.m_execute = [](void* callbackData, CommandList* commandList) {
                static_cast<TFunction*>(callbackData)(commandList);
            };
        }

    private:
        friend FrameGraph;
        friend FrameGraphBuilder;

        FrameGraphPassBuilder(FrameGraph* graph, const uint32_t passIndex)
            : m_graph(graph)
            , m_passIndex(passIndex)
        {
        }

        FrameGraph* m_graph;
        uint32_t m_passIndex;
    };


    struct [[nodiscard]] FrameGraphBuilder final
    {
        FrameGraphPassBuilder AddPass(const Env::Name name, const GraphicsPipelineDesc& pipelineDesc) const
        {
            const uint32_t passIndex = m_graph->AddPassInternal(m_passProducerIndex, name, pipelineDesc);
            return { m_graph, passIndex };
        }

    private:
        friend FrameGraph;

        explicit FrameGraphBuilder(FrameGraph* graph, const uint32_t passProducerIndex)
            : m_graph(graph)
            , m_passProducerIndex(passProducerIndex)
        {
        }

        FrameGraph* m_graph;
        uint32_t m_passProducerIndex;
    };
} // namespace FE::Graphics::RHI
