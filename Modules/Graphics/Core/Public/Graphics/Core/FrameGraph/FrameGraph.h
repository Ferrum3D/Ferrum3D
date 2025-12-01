#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    struct Viewport;


    struct FrameGraphPassBuilder final
    {
    private:
        friend FrameGraph;

        FrameGraphPassBuilder(FrameGraph* graph, const uint32_t passIndex)
            : m_graph(graph)
            , m_passIndex(passIndex)
        {
        }

        FrameGraph* m_graph;
        uint32_t m_passIndex;
    };


    struct FrameGraph : public DeviceObject
    {
        FE_RTTI("EA570124-75F4-4EFC-9C49-69EB5EB0404C");

        std::pmr::memory_resource* GetAllocator()
        {
            return &m_linearAllocator;
        }

        FrameGraphBlackboard& GetBlackboard()
        {
            return m_blackboard;
        }

        template<class T>
        T* AllocatePassData()
        {
            return Memory::New<T>(&m_linearAllocator);
        }

        virtual void RegisterViewport(Viewport* viewport) = 0;
        virtual Viewport* GetViewport() = 0;

        virtual Texture* GetMainColorTarget() const = 0;
        virtual Texture* GetMainDepthStencilTarget() const = 0;

        virtual void BeginFrame() = 0;
        virtual void CompileAndExecute() = 0;

        virtual FrameGraphTextureDescriptorHandle GetDescriptor(const Texture* texture, TextureSubresource subresource) = 0;
        virtual FrameGraphBufferDescriptorHandle GetDescriptor(const Buffer* buffer, BufferSubresource subresource) = 0;
        virtual SamplerDescriptor GetSampler(SamplerState sampler) = 0;

        FrameGraphTextureDescriptorHandle GetDescriptor(const Texture* texture)
        {
            return GetDescriptor(texture, TextureSubresource::CreateWhole(texture->GetDesc()));
        }

        FrameGraphBufferDescriptorHandle GetDescriptor(const Buffer* buffer)
        {
            return GetDescriptor(buffer, BufferSubresource{ 0, buffer->GetDesc().m_size });
        }

        template<class TPassDesc, class TFunctor>
        std::enable_if_t<std::is_invocable_v<TFunctor, FrameGraphContext&>, FrameGraphPassBuilder> AddPass(TPassDesc* passDesc,
                                                                                                           TFunctor&& functor);

    protected:
        friend FrameGraphTextureDescriptorHandle;
        friend FrameGraphBufferDescriptorHandle;

        struct PassFunctor final
        {
            void* m_callbackData = nullptr;
            void (*m_execute)(void* functor, FrameGraphContext& context) = nullptr;
            void (*m_destroy)(void* functor) = nullptr;
        };

        FrameGraph()
            : m_linearAllocator(UINT64_C(64 * 1024), Env::GetStaticAllocator(Memory::StaticAllocatorType::kVirtual))
            , m_blackboard(&m_linearAllocator)
        {
        }

        void SetDescriptorTypeByIndex(uint32_t descriptorIndex, DescriptorType type);
        FrameGraphPassBuilder AddPassInternal(PassFunctor* functor);

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
    };


    template<class TPassDesc, class TFunctor>
    std::enable_if_t<std::is_invocable_v<TFunctor, FrameGraphContext&>, FrameGraphPassBuilder> FrameGraph::AddPass(
        TPassDesc* passDesc, TFunctor&& functor)
    {
        TFunctor* f = Memory::New<TFunctor>(&m_linearAllocator, std::forward<TFunctor>(functor));
        PassFunctor* passFunctor = Memory::New<PassFunctor>(&m_linearAllocator);
        passFunctor->m_callbackData = f;
        passFunctor->m_execute = [](void* callbackData, FrameGraphContext* context) {
            (*static_cast<TFunctor*>(callbackData))(*context);
        };
        passFunctor->m_destroy = [](void* callbackData) {
            static_cast<TFunctor*>(callbackData)->~TFunctor();
        };

        return AddPassInternal(passFunctor);
    }


    inline FrameGraphTextureDescriptorHandle::operator TextureSRVDescriptor() const
    {
        m_graph->SetDescriptorTypeByIndex(m_descriptorIndex, DescriptorType::kSRV);
        return TextureSRVDescriptor{ m_descriptorIndex };
    }


    inline FrameGraphTextureDescriptorHandle::operator TextureUAVDescriptor() const
    {
        m_graph->SetDescriptorTypeByIndex(m_descriptorIndex, DescriptorType::kUAV);
        return TextureUAVDescriptor{ m_descriptorIndex };
    }
} // namespace FE::Graphics::Core


namespace FE::Graphics
{
#define FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(name, baseDescriptor)                                                                    \
    template<class T>                                                                                                            \
    using name = baseDescriptor;

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture3DDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture1DArrayDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(Texture2DArrayDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeDescriptor, TextureSRVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(TextureCubeArrayDescriptor, TextureSRVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture3DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture1DArrayDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(RWTexture2DArrayDescriptor, TextureUAVDescriptor);

    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture3DDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture1DArrayDescriptor, TextureUAVDescriptor);
    FE_FRAME_GRAPH_ALIAS_DESCRIPTOR(GloballyCoherentRWTexture2DArrayDescriptor, TextureUAVDescriptor);

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
