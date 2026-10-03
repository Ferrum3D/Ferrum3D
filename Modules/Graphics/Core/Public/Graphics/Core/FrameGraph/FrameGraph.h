#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/FrameGraph/Blackboard.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Texture.h>
#include <festd/vector.h>

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

        [[nodiscard]] ResourcePool* GetResourcePool() const
        {
            return m_resourcePool;
        }

        [[nodiscard]] DescriptorManager* GetDescriptorManager() const
        {
            return m_descriptorManager;
        }

        virtual void BeginFrame() = 0;
        virtual void CompileAndExecute() = 0;

        //! Returns a final shader index for this graph execution. Reflected fields declare its accesses.
        //! Request views during graph setup; native descriptors are published after resources are committed.
        [[nodiscard]] virtual TextureSRVDescriptor GetSRV(TextureView texture) = 0;
        [[nodiscard]] virtual TextureUAVDescriptor GetUAV(TextureView texture) = 0;
        [[nodiscard]] virtual BufferSRVDescriptor GetSRV(BufferView buffer) = 0;
        [[nodiscard]] virtual BufferUAVDescriptor GetUAV(BufferView buffer) = 0;
        [[nodiscard]] virtual SamplerDescriptor GetSampler(SamplerState sampler) = 0;

        virtual void BeginScope(festd::string_view name) = 0;
        virtual void EndScope() = 0;

        virtual void AddCopyPass(const BufferView& destination, const BufferView& source) = 0;

        template<class TPassDesc>
        BasePassDescToken AddBasePassDesc(const TPassDesc* passDesc);

        template<class TPassDesc>
        void AddPass(festd::string_view name, TPassDesc* passDesc);

        template<class TPassDesc>
        void AddPass(festd::string_view name, TPassDesc* passDesc, BasePassDescToken token);

        template<class TPassDesc, class TFunctor>
        void AddPass(festd::string_view name, TPassDesc* passDesc, TFunctor&& functor);

        template<class TPassDesc, class TFunctor>
        void AddPass(festd::string_view name, TPassDesc* passDesc, BasePassDescToken token, TFunctor&& functor);

        template<class TFunctor>
        void AddPassWithoutBarriers(festd::string_view name, TFunctor&& functor);

        template<class TPassDesc>
        void AddDrawPass(festd::string_view name, TPassDesc* passDesc, uint32_t vertexCount, uint32_t instanceCount = 1,
                         uint32_t vertexOffset = 0, uint32_t instanceOffset = 0);

        template<class TPassDesc>
        void AddDrawIndexedPass(festd::string_view name, TPassDesc* passDesc, uint32_t indexCount, uint32_t instanceCount = 1,
                                uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instanceOffset = 0);

        template<class TPassDesc>
        void AddDispatchPass(festd::string_view name, TPassDesc* passDesc, ComputeWorkGroupCount workGroupCount);

    protected:
        struct PassNodeDesc final
        {
            Env::Name m_name;
            void* m_functor = nullptr;
            void (*m_execute)(void* functor, FrameGraphContext& context) = nullptr;
            void (*m_destroy)(void* functor, void* userPassDesc) = nullptr;

            Rtti::TypeID m_userPassDescTypeID = Rtti::TypeID::kNull;
            void* m_userPassDescPtr = nullptr;
            BasePassDescToken m_basePassDescToken;
        };

        FrameGraph()
            : m_linearAllocator(UINT64_C(64 * 1024), Env::GetStaticAllocator(Memory::StaticAllocatorType::kVirtual))
            , m_blackboard(&m_linearAllocator)
        {
        }

        virtual void AddPassInternal(const PassNodeDesc& desc) = 0;

        struct BasePassDesc final
        {
            const void* m_data = nullptr;
            Rtti::TypeID m_typeID = Rtti::TypeID::kNull;
        };

        DescriptorManager* m_descriptorManager = nullptr;
        ResourcePool* m_resourcePool = nullptr;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
        festd::inline_vector<BasePassDesc, 4> m_basePassDescs;
    };


    template<class TPassDesc>
    BasePassDescToken FrameGraph::AddBasePassDesc(const TPassDesc* passDesc)
    {
        FE_Assert(passDesc);
        BasePassDescToken token;
        token.m_value = static_cast<uint32_t>(m_basePassDescs.size());
        m_basePassDescs.push_back({ passDesc, Rtti::GetTypeID<TPassDesc>() });
        return token;
    }


    template<class TPassDesc>
    void FrameGraph::AddPass(const festd::string_view name, TPassDesc* passDesc)
    {
        AddPass(name, passDesc, BasePassDescToken{});
    }


    template<class TPassDesc>
    void FrameGraph::AddPass(const festd::string_view name, TPassDesc* passDesc, const BasePassDescToken token)
    {
        FE_Assert(passDesc);

        PassNodeDesc desc;
        desc.m_name = Env::Name(name);
        desc.m_functor = nullptr;
        desc.m_userPassDescPtr = passDesc;
        desc.m_userPassDescTypeID = Rtti::GetTypeID<TPassDesc>();
        desc.m_basePassDescToken = token;
        AddPassInternal(desc);
    }


    template<class TPassDesc, class TFunctor>
    void FrameGraph::AddPass(const festd::string_view name, TPassDesc* passDesc, TFunctor&& functor)
    {
        AddPass(name, passDesc, BasePassDescToken{}, std::forward<TFunctor>(functor));
    }


    template<class TPassDesc, class TFunctor>
    void FrameGraph::AddPass(const festd::string_view name, TPassDesc* passDesc, const BasePassDescToken token,
                             TFunctor&& functor)
    {
        FE_Assert(passDesc);
        using FunctorType = std::decay_t<TFunctor>;

        PassNodeDesc desc;
        desc.m_name = Env::Name(name);
        desc.m_functor = Memory::New<FunctorType>(&m_linearAllocator, std::forward<TFunctor>(functor));

        desc.m_execute = [](void* functorPtr, FrameGraphContext& context) {
            static_assert(std::is_invocable_v<FunctorType, FrameGraphContext&>);
            (*static_cast<FunctorType*>(functorPtr))(context);
        };
        desc.m_destroy = [](void* functorPtr, void* descPtr) {
            static_cast<FunctorType*>(functorPtr)->~FunctorType();
            static_cast<TPassDesc*>(descPtr)->~TPassDesc();
        };

        desc.m_userPassDescPtr = passDesc;
        desc.m_userPassDescTypeID = Rtti::GetTypeID<TPassDesc>();
        desc.m_basePassDescToken = token;
        AddPassInternal(desc);
    }


    template<class TFunctor>
    void FrameGraph::AddPassWithoutBarriers(festd::string_view name, TFunctor&& functor)
    {
        using FunctorType = std::decay_t<TFunctor>;

        PassNodeDesc desc;
        desc.m_name = Env::Name(name);
        desc.m_functor = Memory::New<FunctorType>(&m_linearAllocator, std::forward<TFunctor>(functor));

        desc.m_execute = [](void* functorPtr, FrameGraphContext& context) {
            static_assert(std::is_invocable_v<FunctorType, FrameGraphContext&>);
            (*static_cast<FunctorType*>(functorPtr))(context);
        };
        desc.m_destroy = [](void* functorPtr, [[maybe_unused]] void* descPtr) {
            static_cast<FunctorType*>(functorPtr)->~FunctorType();
        };

        desc.m_userPassDescPtr = nullptr;
        desc.m_userPassDescTypeID = Rtti::GetTypeID<EmptyStruct>();
        AddPassInternal(desc);
    }


    template<class TPassDesc>
    void FrameGraph::AddDrawPass(const festd::string_view name, TPassDesc* passDesc, const uint32_t vertexCount,
                                 const uint32_t instanceCount, const uint32_t vertexOffset, const uint32_t instanceOffset)
    {
        AddPass(name, passDesc, [=](FrameGraphContext& context) {
            context.Draw(vertexCount, instanceCount, vertexOffset, instanceOffset);
        });
    }


    template<class TPassDesc>
    void FrameGraph::AddDrawIndexedPass(const festd::string_view name, TPassDesc* passDesc, const uint32_t indexCount,
                                        const uint32_t instanceCount, const uint32_t indexOffset, const uint32_t vertexOffset,
                                        const uint32_t instanceOffset)
    {
        AddPass(name, passDesc, [=](FrameGraphContext& context) {
            context.DrawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
        });
    }


    template<class TPassDesc>
    void FrameGraph::AddDispatchPass(const festd::string_view name, TPassDesc* passDesc,
                                     const ComputeWorkGroupCount workGroupCount)
    {
        AddPass(name, passDesc, [workGroupCount](FrameGraphContext& context) {
            context.Dispatch(workGroupCount);
        });
    }


    struct FrameGraphScope final
    {
        FrameGraphScope(FrameGraph& graph, const festd::string_view name)
            : m_graph(graph)
        {
            graph.BeginScope(name);
        }

        ~FrameGraphScope()
        {
            m_graph.EndScope();
        }

        FrameGraphScope(const FrameGraphScope&) = delete;
        FrameGraphScope& operator=(const FrameGraphScope&) = delete;
        FrameGraphScope(FrameGraphScope&&) = delete;
        FrameGraphScope& operator=(FrameGraphScope&&) = delete;

    private:
        FrameGraph& m_graph;
    };
} // namespace FE::Graphics::Core

#define FE_FG_SCOPE(graph, name) ::FE::Graphics::Core::FrameGraphScope FE_UNIQUE_IDENT(frameGraphScope)(graph, name)
