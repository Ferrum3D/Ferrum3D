#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Device.h>
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


    struct FrameGraph : public Memory::RefCountedObjectBase
    {
        FE_RTTI("EA570124-75F4-4EFC-9C49-69EB5EB0404C");

        [[nodiscard]] Device* GetDevice() const
        {
            return m_device;
        }

        std::pmr::memory_resource* GetAllocator()
        {
            return &m_linearAllocator;
        }

        FrameGraphBlackboard& GetBlackboard()
        {
            return m_blackboard;
        }

        //! Create a blackboard that lives until the graph executes, e.g. to retain one view's data across setup phases.
        FrameGraphBlackboard& CreateBlackboard()
        {
            FrameGraphBlackboard* blackboard = Memory::New<FrameGraphBlackboard>(&m_linearAllocator, &m_linearAllocator);
            m_blackboards.push_back(blackboard);
            return *blackboard;
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

        //! Copy constants into memory that shaders read through the returned pointer during this graph execution.
        //! T must be reflected, as must structs nested in it that hold descriptors: passes that reference the constants
        //! declare the accesses of those descriptors with their own shader stages.
        template<class T>
        [[nodiscard]] ConstantsPtr<T> CreateConstants(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(Rtti::Internal::kIsRTTIDefined<T>, "Constants must be reflected");
            return ConstantsPtr<T>{ CreateConstantsInternal(Rtti::GetType<T>(), &value, 1) };
        }

        //! Like CreateConstants for elements that shaders load one at a time with ConstantsPtr::LoadElement.
        //!
        //! Prefer this to an array field. Loading a struct through a device address and then indexing its array in a loop made
        //! SunShadowData over a thousand times slower on an NVIDIA GPU than loading only the element it needed.
        template<class T>
        [[nodiscard]] ConstantsPtr<T> CreateConstantsArray(const festd::span<const T> values)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(Rtti::Internal::kIsRTTIDefined<T>, "Constants must be reflected");
            return ConstantsPtr<T>{ CreateConstantsInternal(Rtti::GetType<T>(), values.data(), values.size()) };
        }

        virtual void BeginScope(festd::string_view name) = 0;
        virtual void EndScope() = 0;

        virtual void AddCopyPass(const BufferView& destination, const BufferView& source) = 0;

        //! Copy host-written data before the first pass, batched with the other uploads of the frame. One barrier then
        //! covers the reads that consume the uploads. Falls back to a copy pass once the destination is accessed this frame.
        virtual void AddUploadCopy(const BufferView& destination, const BufferView& source) = 0;

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

        //! Add a compute pass that needs nothing but its push constants, whose descriptors declare its accesses.
        template<class TConstants>
        void AddDispatch(festd::string_view name, const ComputePipeline* pipeline, const TConstants& constants,
                         ComputeWorkGroupCount workGroupCount);

        template<class TConstants>
        void AddDispatchIndirect(festd::string_view name, const ComputePipeline* pipeline, const TConstants& constants,
                                 BufferView arguments);

    protected:
        struct PassNodeDesc final
        {
            Env::Name m_name;
            BasePassDescToken m_basePassDescToken;

            void* m_functor = nullptr;
            void (*m_execute)(void* functor, FrameGraphContext& context) = nullptr;
            void (*m_destroy)(void* functor, void* userPassDesc) = nullptr;

            Rtti::TypeID m_userPassDescTypeID = Rtti::TypeID::kNull;
            void* m_userPassDescPtr = nullptr;

            //! Set for compute passes without a pass desc.
            const ComputePipeline* m_computePipeline = nullptr;
            const Rtti::Type* m_constantsType = nullptr;
            const void* m_constants = nullptr;
            BufferView m_indirectArgs = BufferView::kInvalid;
        };

        FrameGraph()
            // The pass, resource and scope arrays of a frame grow inside single pages, so a page must hold the largest of them;
            // views with many child views, e.g. shadow maps, record thousands of passes.
            : m_linearAllocator(UINT64_C(1024 * 1024), Env::GetStaticAllocator(Memory::StaticAllocatorType::kVirtual))
            , m_blackboard(&m_linearAllocator)
        {
        }

        virtual void AddPassInternal(const PassNodeDesc& desc) = 0;
        [[nodiscard]] virtual AnyConstantsPtr CreateConstantsInternal(const Rtti::Type& type, const void* data,
                                                                      uint32_t count) = 0;

        template<class TConstants, class TFunctor>
        void AddComputePass(festd::string_view name, const ComputePipeline* pipeline, const TConstants& constants,
                            BufferView indirectArgs, TFunctor&& functor);

        struct BasePassDesc final
        {
            const void* m_data = nullptr;
            Rtti::TypeID m_typeID = Rtti::TypeID::kNull;
        };

        Device* m_device = nullptr;
        DescriptorManager* m_descriptorManager = nullptr;
        ResourcePool* m_resourcePool = nullptr;

        Memory::LinearAllocator m_linearAllocator;
        FrameGraphBlackboard m_blackboard;
        festd::inline_vector<FrameGraphBlackboard*, 8> m_blackboards;
        festd::inline_vector<BasePassDesc, 4> m_basePassDescs;
    };


    template<class TPassDesc>
    BasePassDescToken FrameGraph::AddBasePassDesc(const TPassDesc* passDesc)
    {
        FE_Assert(passDesc);

        BasePassDescToken token;
        token.m_value = m_basePassDescs.size();
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


    template<class TConstants, class TFunctor>
    void FrameGraph::AddComputePass(const festd::string_view name, const ComputePipeline* pipeline, const TConstants& constants,
                                    const BufferView indirectArgs, TFunctor&& functor)
    {
        static_assert(std::is_trivially_copyable_v<TConstants> && std::is_standard_layout_v<TConstants>);
        static_assert(Rtti::Internal::kIsRTTIDefined<TConstants>, "Push constants must be reflected");
        FE_Assert(pipeline);

        using FunctorType = std::decay_t<TFunctor>;

        PassNodeDesc desc;
        desc.m_name = Env::Name(name);
        desc.m_functor = Memory::New<FunctorType>(&m_linearAllocator, std::forward<TFunctor>(functor));
        desc.m_execute = [](void* functorPtr, FrameGraphContext& context) {
            (*static_cast<FunctorType*>(functorPtr))(context);
        };
        desc.m_destroy = [](void* functorPtr, [[maybe_unused]] void* descPtr) {
            static_cast<FunctorType*>(functorPtr)->~FunctorType();
        };

        desc.m_userPassDescTypeID = Rtti::GetTypeID<EmptyStruct>();
        desc.m_computePipeline = pipeline;
        desc.m_constantsType = &Rtti::GetType<TConstants>();
        desc.m_constants = Memory::New<TConstants>(&m_linearAllocator, constants);
        desc.m_indirectArgs = indirectArgs;
        AddPassInternal(desc);
    }


    template<class TConstants>
    void FrameGraph::AddDispatch(const festd::string_view name, const ComputePipeline* pipeline, const TConstants& constants,
                                 const ComputeWorkGroupCount workGroupCount)
    {
        AddComputePass(name, pipeline, constants, BufferView::kInvalid, [workGroupCount](FrameGraphContext& context) {
            context.Dispatch(workGroupCount);
        });
    }


    template<class TConstants>
    void FrameGraph::AddDispatchIndirect(const festd::string_view name, const ComputePipeline* pipeline,
                                         const TConstants& constants, const BufferView arguments)
    {
        FE_Assert(arguments.IsValid());
        AddComputePass(name, pipeline, constants, arguments, [](FrameGraphContext& context) {
            context.DispatchIndirect();
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
