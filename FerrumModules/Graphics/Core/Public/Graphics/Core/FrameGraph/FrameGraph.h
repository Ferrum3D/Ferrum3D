#pragma once
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/Image.h>

namespace FE::Graphics::Core
{
    struct Viewport;


    struct PassProducer : public Memory::RefCountedObjectBase
    {
        virtual void Setup(FrameGraph& graph, FrameGraphBuilder& builder, FrameGraphBlackboard& blackboard) = 0;
    };


    struct FrameGraph : public DeviceObject
    {
        FE_RTTI_Class(FrameGraph, "EA570124-75F4-4EFC-9C49-69EB5EB0404C");

        std::pmr::memory_resource* GetAllocator()
        {
            return &m_linearAllocator;
        }

        FrameGraphBlackboard& GetBlackboard()
        {
            return m_blackboard;
        }

        virtual void RegisterViewport(Viewport* viewport) = 0;
        virtual Viewport* GetViewport() = 0;

        virtual ImageHandle GetRenderTarget() const = 0;
        virtual ImageHandle GetDepthStencil() const = 0;

        virtual void AddPassProducer(PassProducer* passProducer) = 0;
        virtual void Execute() = 0;

        virtual Image* GetImage(ImageHandle image) const = 0;
        virtual Buffer* GetBuffer(BufferHandle buffer) const = 0;

        virtual ImageHandle ImportImage(Image* image) = 0;
        virtual BufferHandle ImportBuffer(Buffer* buffer) = 0;

    protected:
        friend FrameGraphBuilder;
        friend FrameGraphPassBuilder;

        FrameGraph()
            : m_linearAllocator(UINT64_C(64 * 1024), Env::GetStaticAllocator(Memory::StaticAllocatorType::kVirtual))
            , m_blackboard(&m_linearAllocator)
            , m_passProducers(&m_linearAllocator)
        {
        }

        struct PassDataBase
        {
            void* m_executeCallbackData = nullptr;
            void (*m_execute)(void* callbackData, FrameGraphContext* context) = nullptr;
            void (*m_callbackDestructor)(void* callbackData) = nullptr;

            ~PassDataBase()
            {
                if (m_callbackDestructor)
                    m_callbackDestructor(m_executeCallbackData);

                m_executeCallbackData = nullptr;
                m_callbackDestructor = nullptr;
                m_execute = nullptr;
            }
        };

        virtual PassDataBase& GetPassData(uint32_t passIndex) = 0;
        virtual uint32_t AddPassInternal(uint32_t producerIndex, Env::Name name) = 0;

        virtual BufferHandle CreateBuffer(uint32_t passIndex, Env::Name name, const BufferDesc& desc) = 0;
        virtual ImageHandle CreateImage(uint32_t passIndex, Env::Name name, const ImageDesc& desc) = 0;

        virtual uint32_t ReadResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) = 0;
        virtual uint32_t WriteResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) = 0;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
        SegmentedVector<Rc<PassProducer>> m_passProducers;
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
            FE_Assert(buffer.IsValid());
            const uint32_t flags = festd::to_underlying(readType);
            const uint32_t newVersion = m_graph->ReadResource(m_passIndex, buffer.m_desc.m_resourceIndex, flags);
            return BufferHandle::Create(buffer.m_desc.m_resourceIndex, newVersion);
        }

        ImageHandle Read(const ImageHandle image, const ImageReadType readType) const
        {
            FE_Assert(image.IsValid());
            const uint32_t flags = festd::to_underlying(readType);
            const uint32_t newVersion = m_graph->ReadResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return ImageHandle::Create(image.m_desc.m_resourceIndex, newVersion);
        }

        BufferHandle Write(const BufferHandle buffer, const BufferWriteType writeType) const
        {
            FE_Assert(buffer.IsValid());
            const uint32_t flags = festd::to_underlying(writeType);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, buffer.m_desc.m_resourceIndex, flags);
            return BufferHandle::Create(buffer.m_desc.m_resourceIndex, newVersion);
        }

        ImageHandle Write(const ImageHandle image, const ImageWriteType writeType) const
        {
            FE_Assert(image.IsValid());
            const uint32_t flags = festd::to_underlying(writeType);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return ImageHandle::Create(image.m_desc.m_resourceIndex, newVersion);
        }

        template<class TFunction>
        void SetFunction(TFunction&& function) const
        {
            TFunction* f = Memory::New<TFunction>(&m_graph->m_linearAllocator, festd::forward<TFunction>(function));

            auto& passData = m_graph->GetPassData(m_passIndex);
            passData.m_executeCallbackData = f;
            passData.m_execute = [](void* callbackData, FrameGraphContext* context) {
                (*static_cast<TFunction*>(callbackData))(context);
            };
            passData.m_callbackDestructor = [](void* callbackData) {
                static_cast<TFunction*>(callbackData)->~TFunction();
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
        FrameGraphBuilder(FrameGraph* graph, const uint32_t passProducerIndex)
            : m_graph(graph)
            , m_passProducerIndex(passProducerIndex)
        {
        }

        FrameGraphPassBuilder AddPass(const Env::Name name) const
        {
            const uint32_t passIndex = m_graph->AddPassInternal(m_passProducerIndex, name);
            return { m_graph, passIndex };
        }

    private:
        FrameGraph* m_graph;
        uint32_t m_passProducerIndex;
    };
} // namespace FE::Graphics::Core
