#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Common/ResourceBarrierBatcher.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Common
{
    enum class PassStateFlags
    {
        kNone = 0,
        kPushConstants = 1 << 0,
        kColorTarget = 1 << 1,
        kDepthTarget = 1 << 2,
        kViewport = 1 << 3,
        kScissor = 1 << 4,
        kGraphicsPipeline = 1 << 5,
        kComputePipeline = 1 << 6,
    };

    FE_ENUM_OPERATORS(PassStateFlags);


    struct FrameGraph : public Core::FrameGraph
    {
        FE_RTTI("39F873DC-8F3D-4821-BA66-92FCD380B69A");

        Core::ResourcePool* GetResourcePool() override;

        void BeginFrame() override;
        void CompileAndExecute() override;

        Core::FrameGraphTextureDescriptorHandle GetDescriptor(Core::TextureView texture) override;
        Core::FrameGraphBufferDescriptorHandle GetDescriptor(Core::BufferView buffer) override;
        SamplerDescriptor GetSampler(Core::SamplerState sampler) override;

        void AddCopyPass(const Core::BufferView& destination, const Core::BufferView& source) override;

    protected:
        struct TextureAccess final
        {
            uint32_t m_localResourceIndex = kInvalidIndex;
            Core::BarrierSyncFlags m_syncFlags = Core::BarrierSyncFlags::kNone;
            Core::BarrierAccessFlags m_accessFlags = Core::BarrierAccessFlags::kNone;
            Core::BarrierLayout m_layout = Core::BarrierLayout::kUndefined;
            Core::TextureSubresource m_subresource = Core::TextureSubresource::kInvalid;
        };

        struct BufferAccess final
        {
            uint32_t m_localResourceIndex = kInvalidIndex;
            Core::BarrierSyncFlags m_syncFlags = Core::BarrierSyncFlags::kNone;
            Core::BarrierAccessFlags m_accessFlags = Core::BarrierAccessFlags::kNone;
        };

        struct PassNode final
        {
            explicit PassNode(std::pmr::memory_resource* allocator);

            Env::Name m_name;
            uint32_t m_passIndex = kInvalidIndex;

            void* m_functor = nullptr;
            void (*m_execute)(void* functor, Core::FrameGraphContext& context) = nullptr;
            void (*m_destroy)(void* functor, void* userPassDesc) = nullptr;

            Rtti::TypeID m_userPassDescTypeID = Rtti::TypeID::kNull;
            void* m_userPassDescPtr = nullptr;

            festd::pmr::vector<Core::TextureBarrierDesc> m_textureOwnershipTransferBarriers;
            festd::pmr::vector<Core::BufferBarrierDesc> m_bufferOwnershipTransferBarriers;

            ResourceBarrierBatcher m_barrierBatcher;

            festd::pmr::inline_vector<TextureAccess, 4> m_accessedTextures;
            festd::pmr::inline_vector<BufferAccess, 4> m_accessedBuffers;

            PassStateFlags m_specifiedStatesMask = PassStateFlags::kNone;

            const Core::PipelineBase* m_pipeline = nullptr;

            festd::span<const std::byte> m_pushConstants;
            Rtti::TypeID m_pushConstantsTypeID = Rtti::TypeID::kNull;

            festd::fixed_vector<uint32_t, Core::Limits::Pipeline::kMaxColorAttachments> m_colorTargetAccessIndices;
            uint32_t m_depthTargetAccessIndex = kInvalidIndex;

            RectF m_viewport{ kForceInit };
            RectInt m_scissor{ kForceInit };
        };

        struct PassResourceAccess final
        {
            uint32_t m_passIndex = kInvalidIndex;
            uint32_t m_accessIndex = kInvalidIndex;
        };

        struct ResourceNode final
        {
            explicit ResourceNode(std::pmr::memory_resource* allocator);

            Rc<Core::Resource> m_resource;
            festd::pmr::inline_vector<PassResourceAccess, 4> m_accesses;

            uint32_t m_previousBarrierPassIndex = kInvalidIndex;
            Core::BarrierAccessFlags m_bindFlags = Core::BarrierAccessFlags::kNone;
            bool m_isOwnedByGraph = false;
        };

        FrameGraph(Core::Device* device, Core::DescriptorManager* descriptorManager, Core::ResourcePool* resourcePool);

        void AddPassInternal(const PassNodeDesc& desc) override;

        virtual void PrepareExecuteInternal() = 0;
        virtual void FinishExecuteInternal() = 0;
        virtual void ExecutePassBarriersInternal(PassNode& pass) = 0;

        void ParsePassPushConstants(PassNode& pass, const Rtti::Type& type);
        void PreparePassCompileInfo(PassNode& pass);
        void AccumulateBindFlags(ResourceNode& resource);
        void CompilePassBarriers(PassNode& pass);

        void Compile();
        void Execute();

        uint32_t RegisterResource(Core::Resource* resource, PassNode& pass, const TextureAccess& access);
        uint32_t RegisterResource(Core::Resource* resource, PassNode& pass, const BufferAccess& access);

        Core::DescriptorManager* m_descriptorManager = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;
        Rc<Core::FrameGraphContext> m_currentContext;

        SegmentedVector<PassNode> m_passes;
        SegmentedVector<ResourceNode> m_resources;
        festd::segmented_unordered_dense_map<uint32_t, uint32_t> m_resourceIndexMap;
    };
} // namespace FE::Graphics::Common
