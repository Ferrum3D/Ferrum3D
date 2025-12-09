#pragma once
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
    struct ResourcePool;


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

        virtual ResourcePool* GetResourcePool() = 0;

        virtual void BeginFrame() = 0;
        virtual void CompileAndExecute() = 0;

        virtual FrameGraphTextureDescriptorHandle GetDescriptor(TextureView texture) = 0;
        virtual FrameGraphBufferDescriptorHandle GetDescriptor(BufferView buffer) = 0;
        virtual SamplerDescriptor GetSampler(SamplerState sampler) = 0;

        FrameGraphTextureDescriptorHandle GetDescriptor(Texture* texture)
        {
            return GetDescriptor(TextureView::Create(texture));
        }

        FrameGraphBufferDescriptorHandle GetDescriptor(Buffer* buffer)
        {
            return GetDescriptor(BufferView::Create(buffer));
        }

        template<class TPassDesc, class TFunctor>
        void AddPass(Env::Name name, TPassDesc* passDesc, TFunctor&& functor);

        template<class TPassDesc>
        void AddDrawIndexedInstancedPass(Env::Name name, TPassDesc* passDesc, uint32_t vertexCount, uint32_t instanceCount = 1,
                                         uint32_t vertexOffset = 0, uint32_t instanceOffset = 0);

        template<class TPassDesc>
        void AddDispatchPass(Env::Name name, TPassDesc* passDesc, ComputeWorkGroupCount workGroupCount);

    protected:
        friend FrameGraphTextureDescriptorHandle;
        friend FrameGraphBufferDescriptorHandle;

        struct PassNodeDesc final
        {
            Env::Name m_name;
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

        virtual void AddPassInternal(const PassNodeDesc& desc) = 0;
        virtual void CommitPassNode(uint32_t passIndex) = 0;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
    };


    template<class TPassDesc, class TFunctor>
    void FrameGraph::AddPass(const Env::Name name, TPassDesc* passDesc, TFunctor&& functor)
    {
        TFunctor* f = Memory::New<TFunctor>(&m_linearAllocator, std::forward<TFunctor>(functor));
        PassNodeDesc desc;
        desc.m_name = name;
        desc.m_functor = f;

        desc.m_execute = [](void* functorPtr, FrameGraphContext& context) {
            static_assert(std::is_invocable_v<TFunctor, FrameGraphContext&>);
            (*static_cast<TFunctor*>(functorPtr))(context);
        };
        desc.m_destroy = [](void* functorPtr, void* descPtr) {
            static_cast<TFunctor*>(functorPtr)->~TFunctor();
            static_cast<TPassDesc*>(descPtr)->~TPassDesc();
        };

        desc.m_userPassDescPtr = passDesc;
        desc.m_userPassDescTypeID = RTTI::GetTypeID<TPassDesc>();
        FE_Assert(desc.m_userPassDescTypeID != RTTI::TypeID::kNull, "PassDesc must be registered with RTTI");

        AddPassInternal(desc);
    }


    template<class TPassDesc>
    void FrameGraph::AddDrawIndexedInstancedPass(const Env::Name name, TPassDesc* passDesc, const uint32_t vertexCount,
                                                 const uint32_t instanceCount, const uint32_t vertexOffset,
                                                 const uint32_t instanceOffset)
    {
        AddPass(name, passDesc, [=](FrameGraphContext& context) {
            context.DrawIndexedInstanced(vertexCount, instanceCount, vertexOffset, instanceOffset);
        });
    }


    template<class TPassDesc>
    void FrameGraph::AddDispatchPass(const Env::Name name, TPassDesc* passDesc, const ComputeWorkGroupCount workGroupCount)
    {
        AddPass(name, passDesc, [workGroupCount](FrameGraphContext& context) {
            context.Dispatch(workGroupCount);
        });
    }
} // namespace FE::Graphics::Core
