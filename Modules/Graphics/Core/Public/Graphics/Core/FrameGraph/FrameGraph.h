#pragma once
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
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

        void BeginScope(festd::string_view name);
        void EndScope();

        virtual void AddCopyPass(const BufferView& destination, const BufferView& source) = 0;

        template<class TPassDesc, class TFunctor>
        void AddPass(festd::string_view name, TPassDesc* passDesc, TFunctor&& functor);

        template<class TPassDesc>
        void AddDrawPass(festd::string_view name, TPassDesc* passDesc, uint32_t vertexCount, uint32_t instanceCount = 1,
                         uint32_t vertexOffset = 0, uint32_t instanceOffset = 0);

        template<class TPassDesc>
        void AddDrawIndexedPass(festd::string_view name, TPassDesc* passDesc, uint32_t indexCount, uint32_t instanceCount = 1,
                                uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instanceOffset = 0);

        template<class TPassDesc>
        void AddDispatchPass(festd::string_view name, TPassDesc* passDesc, ComputeWorkGroupCount workGroupCount);

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

        Env::Name FormatPassName(festd::string_view name);

        virtual void AddPassInternal(const PassNodeDesc& desc) = 0;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
        festd::fixed_string m_currentScope;
    };


    inline void FrameGraph::BeginScope(const festd::string_view name)
    {
        if (Build::IsDebug())
        {
            for (const int32_t codepoint : name)
            {
                const char c = static_cast<char>(codepoint);
                FE_Assert(ASCII::IsValid(codepoint) && (ASCII::IsLetter(c) || ASCII::IsDigit(c)));
            }
        }

        if (!m_currentScope.empty())
            m_currentScope += "/";
        m_currentScope += name;
    }


    inline void FrameGraph::EndScope()
    {
        const auto it = m_currentScope.find_last_of('/');
        if (it == m_currentScope.end())
        {
            m_currentScope.clear();
            return;
        }

        m_currentScope.resize(static_cast<uint32_t>(it.m_iter - m_currentScope.data()), 0);
    }


    inline Env::Name FrameGraph::FormatPassName(const festd::string_view name)
    {
        if (m_currentScope.empty())
            return Env::Name(name);

        return Fmt::FormatName("{}/{}", m_currentScope, name);
    }


    template<class TPassDesc, class TFunctor>
    void FrameGraph::AddPass(const festd::string_view name, TPassDesc* passDesc, TFunctor&& functor)
    {
        FE_Assert(passDesc);

        PassNodeDesc desc;
        desc.m_name = FormatPassName(name);
        desc.m_functor = Memory::New<TFunctor>(&m_linearAllocator, std::forward<TFunctor>(functor));

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
