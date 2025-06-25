#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/RenderTarget.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Texture.h>

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

        virtual RenderTargetHandle GetMainColorTarget() const = 0;
        virtual RenderTargetHandle GetMainDepthStencilTarget() const = 0;

        virtual void AddPassProducer(PassProducer* passProducer) = 0;
        virtual void Execute() = 0;

        virtual RenderTarget* GetRenderTarget(RenderTargetHandle image) const = 0;
        virtual Buffer* GetBuffer(BufferHandle buffer) const = 0;

        virtual ImageDesc GetResourceDesc(RenderTargetHandle image) const = 0;
        virtual BufferDesc GetResourceDesc(BufferHandle buffer) const = 0;

        virtual Env::Name GetResourceName(RenderTargetHandle image) const = 0;
        virtual Env::Name GetResourceName(BufferHandle buffer) const = 0;

        virtual RenderTargetHandle ImportRenderTarget(RenderTarget* image, ImageAccessType access) = 0;
        virtual BufferHandle ImportBuffer(Buffer* buffer, BufferAccessType access) = 0;

        virtual ImageSRVDescriptor GetSRV(const Texture* texture, ImageSubresource subresource) = 0;
        virtual ImageSRVDescriptor GetSRV(const RenderTarget* texture, ImageSubresource subresource) = 0;
        virtual ImageUAVDescriptor GetUAV(const RenderTarget* texture, ImageSubresource subresource) = 0;
        virtual BufferSRVDescriptor GetSRV(const Buffer* buffer, uint32_t offset, uint32_t size) = 0;
        virtual BufferUAVDescriptor GetUAV(const Buffer* buffer, uint32_t offset, uint32_t size) = 0;
        virtual SamplerDescriptor GetSampler(SamplerState sampler) = 0;

        ImageSRVDescriptor GetSRV(const Texture* texture)
        {
            return GetSRV(texture, ImageSubresource::kInvalid);
        }

        ImageSRVDescriptor GetSRV(const RenderTarget* texture)
        {
            return GetSRV(texture, ImageSubresource::kInvalid);
        }

        ImageUAVDescriptor GetUAV(const RenderTarget* texture)
        {
            return GetUAV(texture, ImageSubresource::kInvalid);
        }

        BufferSRVDescriptor GetSRV(const Buffer* buffer)
        {
            return GetSRV(buffer, 0, buffer->GetDesc().m_size);
        }

        BufferUAVDescriptor GetUAV(const Buffer* buffer)
        {
            return GetUAV(buffer, 0, buffer->GetDesc().m_size);
        }

        ImageSRVDescriptor GetSRV(const RenderTargetHandle image, const ImageSubresource subresource)
        {
            return GetSRV(GetRenderTarget(image), subresource);
        }

        ImageUAVDescriptor GetUAV(const RenderTargetHandle image, const ImageSubresource subresource)
        {
            return GetUAV(GetRenderTarget(image), subresource);
        }

        ImageSRVDescriptor GetSRV(const RenderTargetHandle image)
        {
            return GetSRV(GetRenderTarget(image));
        }

        ImageUAVDescriptor GetUAV(const RenderTargetHandle image)
        {
            return GetUAV(GetRenderTarget(image));
        }

        BufferSRVDescriptor GetSRV(const BufferHandle buffer)
        {
            return GetSRV(GetBuffer(buffer));
        }

        BufferUAVDescriptor GetUAV(const BufferHandle buffer)
        {
            return GetUAV(GetBuffer(buffer));
        }

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
        virtual RenderTargetHandle CreateImage(uint32_t passIndex, Env::Name name, const ImageDesc& desc) = 0;

        virtual uint32_t ReadResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) = 0;
        virtual uint32_t WriteResource(uint32_t passIndex, uint32_t resourceIndex, uint32_t flags) = 0;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
        SegmentedVector<PassProducer*> m_passProducers;
    };


    struct [[nodiscard]] FrameGraphPassBuilder final
    {
        BufferHandle CreateBuffer(const Env::Name name, const BufferDesc& desc) const
        {
            return m_graph->CreateBuffer(m_passIndex, name, desc);
        }

        template<class T>
        BufferHandle CreateStructuredBuffer(const Env::Name name, const uint32_t elementCount) const
        {
            BufferDesc desc;
            desc.m_size = sizeof(T) * elementCount;
            desc.m_flags = BindFlags::kUnorderedAccess | BindFlags::kShaderResource;
            desc.m_usage = ResourceUsage::kDeviceOnly;
            return CreateBuffer(name, desc);
        }

        RenderTargetHandle CreateImage(const Env::Name name, const ImageDesc& desc) const
        {
            return m_graph->CreateImage(m_passIndex, name, desc);
        }

        BufferHandle Read(const BufferHandle buffer, const BufferReadType readType) const
        {
            FE_Assert(buffer.IsValid());
            const uint32_t flags = festd::to_underlying(readType);
            const uint32_t newVersion = m_graph->ReadResource(m_passIndex, buffer.m_desc.m_resourceIndex, flags);
            return BufferHandle::Create(buffer.m_desc.m_resourceIndex, newVersion, flags);
        }

        RenderTargetHandle Read(const RenderTargetHandle image, const ImageReadType readType) const
        {
            FE_Assert(image.IsValid());
            const uint32_t flags = festd::to_underlying(readType);
            const uint32_t newVersion = m_graph->ReadResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return RenderTargetHandle::Create(image.m_desc.m_resourceIndex, newVersion, flags);
        }

        BufferHandle Write(const BufferHandle buffer) const
        {
            FE_Assert(buffer.IsValid());
            constexpr uint32_t flags = festd::to_underlying(BufferWriteType::kUnorderedAccess);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, buffer.m_desc.m_resourceIndex, flags);
            return BufferHandle::Create(buffer.m_desc.m_resourceIndex, newVersion, flags);
        }

        RenderTargetHandle WriteRenderTarget(const RenderTargetHandle image) const
        {
            FE_Assert(image.IsValid());
            const ImageDesc desc = m_graph->GetResourceDesc(image);
            const FormatInfo formatInfo{ desc.m_imageFormat };
            const ImageWriteType writeType = formatInfo.m_aspectFlags == ImageAspect::kColor
                ? ImageWriteType::kColorTarget
                : ImageWriteType::kDepthStencilTarget;
            const uint32_t flags = festd::to_underlying(writeType);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return RenderTargetHandle::Create(image.m_desc.m_resourceIndex, newVersion, flags);
        }

        RenderTargetHandle WriteUAV(const RenderTargetHandle image) const
        {
            FE_Assert(image.IsValid());
            const uint32_t flags = festd::to_underlying(ImageWriteType::kUnorderedAccess);
            const uint32_t newVersion = m_graph->WriteResource(m_passIndex, image.m_desc.m_resourceIndex, flags);
            return RenderTargetHandle::Create(image.m_desc.m_resourceIndex, newVersion, flags);
        }

        template<class TFunction>
        void SetFunction(TFunction&& function) const
        {
            TFunction* f = Memory::New<TFunction>(&m_graph->m_linearAllocator, std::forward<TFunction>(function));

            auto& passData = m_graph->GetPassData(m_passIndex);
            passData.m_executeCallbackData = f;
            passData.m_execute = [](void* callbackData, FrameGraphContext* context) {
                (*static_cast<TFunction*>(callbackData))(*context);
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

        [[nodiscard]] FrameGraph& GetGraph() const
        {
            return *m_graph;
        }

    private:
        FrameGraph* m_graph;
        uint32_t m_passProducerIndex;
    };
} // namespace FE::Graphics::Core


namespace FE::Graphics
{
#define FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(name, baseDescriptor)                                                                    \
    template<class T>                                                                                                            \
    using name = baseDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DDescriptor, ImageSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DDescriptor, ImageSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture3DDescriptor, ImageSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DArrayDescriptor, ImageSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DArrayDescriptor, ImageSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeDescriptor, ImageSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeArrayDescriptor, ImageSRVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture3DDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DArrayDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DArrayDescriptor, ImageUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture3DDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DArrayDescriptor, ImageUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DArrayDescriptor, ImageUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(BufferDescriptor, BufferSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(StructuredBufferDescriptor, BufferSRVDescriptor);
    using ByteAddressBufferDescriptor = BufferSRVDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWBufferDescriptor, BufferUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWStructuredBufferDescriptor, BufferUAVDescriptor);
    using RWByteAddressBufferDescriptor = BufferUAVDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentStructuredBufferDescriptor, BufferUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWStructuredBufferDescriptor, BufferUAVDescriptor);
    using GloballyCoherentRWByteAddressBufferDescriptor = BufferUAVDescriptor;

#undef FE_FRAME_GRAPH_ALIAS_DESCRIPTOR
} // namespace FE::Graphics
