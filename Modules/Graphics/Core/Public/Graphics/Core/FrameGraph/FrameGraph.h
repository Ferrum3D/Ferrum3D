#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
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

        virtual void BeginFrame() = 0;
        virtual void CompileAndExecute() = 0;

        virtual FrameGraphTextureDescriptorHandle GetDescriptor(Texture* texture, TextureSubresource subresource) = 0;
        virtual FrameGraphBufferDescriptorHandle GetDescriptor(Buffer* buffer, BufferSubresource subresource) = 0;
        virtual SamplerDescriptor GetSampler(SamplerState sampler) = 0;

        FrameGraphTextureDescriptorHandle GetDescriptor(Texture* texture)
        {
            return GetDescriptor(texture, TextureSubresource::CreateWhole(texture->GetDesc()));
        }

        FrameGraphBufferDescriptorHandle GetDescriptor(Buffer* buffer)
        {
            return GetDescriptor(buffer, BufferSubresource{ 0, buffer->GetDesc().m_size });
        }

        template<class TPassDesc, class TFunctor>
        FrameGraphPassBuilder AddPass(Env::Name name, TPassDesc* passDesc, TFunctor&& functor);

        template<class TPassDesc>
        FrameGraphPassBuilder AddDrawIndexedInstancedPass(Env::Name name, TPassDesc* passDesc, uint32_t vertexCount,
                                                          uint32_t instanceCount = 1, uint32_t vertexOffset = 0,
                                                          uint32_t instanceOffset = 0);

        template<class TPassDesc>
        FrameGraphPassBuilder AddDispatchPass(Env::Name name, TPassDesc* passDesc, ComputeWorkGroupCount workGroupCount);

    protected:
        friend FrameGraphTextureDescriptorHandle;
        friend FrameGraphBufferDescriptorHandle;

        struct PassNodeBase
        {
            Env::Name m_name;
            uint32_t m_passIndex = kInvalidIndex;
            void* m_functor = nullptr;
            void (*m_execute)(void* functor, FrameGraphContext& context) = nullptr;
            void (*m_destroy)(void* functor, void* userPassDesc) = nullptr;

            RTTI::TypeID m_userPassDescTypeID = RTTI::TypeID::kNull;
            void* m_userPassDescPtr = nullptr;
        };

        FrameGraph()
            : m_linearAllocator(UINT64_C(64 * 1024), Env::GetStaticAllocator(Memory::StaticAllocatorType::kVirtual))
            , m_blackboard(&m_linearAllocator)
        {
        }

        virtual PassNodeBase& AddPassInternal() = 0;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
    };


    template<class TPassDesc, class TFunctor>
    FrameGraphPassBuilder FrameGraph::AddPass(const Env::Name name, TPassDesc* passDesc, TFunctor&& functor)
    {
        TFunctor* f = Memory::New<TFunctor>(&m_linearAllocator, std::forward<TFunctor>(functor));
        PassNodeBase& passNode = AddPassInternal();
        passNode.m_name = name;
        passNode.m_functor = f;
        passNode.m_execute = [](void* functorPtr, FrameGraphContext& context) {
            static_assert(std::is_invocable_v<TFunctor, FrameGraphContext&>);
            (*static_cast<TFunctor*>(functorPtr))(context);
        };
        passNode.m_destroy = [](void* functorPtr, void* descPtr) {
            static_cast<TFunctor*>(functorPtr)->~TFunctor();
            static_cast<TPassDesc*>(descPtr)->~TPassDesc();
        };
        passNode.m_userPassDescTypeID = RTTI::GetTypeID<TPassDesc>();
        passNode.m_userPassDescPtr = passDesc;

        FE_Assert(passNode.m_userPassDescTypeID != RTTI::TypeID::kNull, "PassDesc must be registered with RTTI");

        return FrameGraphPassBuilder{ this, passNode.m_passIndex };
    }


    template<class TPassDesc>
    FrameGraphPassBuilder FrameGraph::AddDrawIndexedInstancedPass(const Env::Name name, TPassDesc* passDesc,
                                                                  const uint32_t vertexCount, const uint32_t instanceCount,
                                                                  const uint32_t vertexOffset, const uint32_t instanceOffset)
    {
        return AddPass(name, passDesc, [=](FrameGraphContext& context) {
            context.DrawIndexedInstanced(vertexCount, instanceCount, vertexOffset, instanceOffset);
        });
    }


    template<class TPassDesc>
    FrameGraphPassBuilder FrameGraph::AddDispatchPass(const Env::Name name, TPassDesc* passDesc,
                                                      const ComputeWorkGroupCount workGroupCount)
    {
        return AddPass(name, passDesc, [workGroupCount](FrameGraphContext& context) {
            context.Dispatch(workGroupCount);
        });
    }
} // namespace FE::Graphics::Core
