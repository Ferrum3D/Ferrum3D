#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Common
{
    enum class PassStateFlags final
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

        void CompileAndExecute() override;

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

        struct PassNode final : public PassNodeBase
        {
            explicit PassNode(std::pmr::memory_resource* allocator);

            festd::pmr::inline_vector<Core::TextureBarrierDesc, 4> m_textureBarriers;
            festd::pmr::inline_vector<Core::BufferBarrierDesc, 4> m_bufferBarriers;

            festd::pmr::inline_vector<TextureAccess, 4> m_accessedTextures;
            festd::pmr::inline_vector<BufferAccess, 4> m_accessedBuffers;

            PassStateFlags m_specifiedStatesMask = PassStateFlags::kNone;

            const Core::PipelineBase* m_pipeline = nullptr;

            festd::span<const std::byte> m_pushConstants;
            RTTI::TypeID m_pushConstantsTypeID = RTTI::TypeID::kNull;

            festd::array<uint32_t, Core::Limits::Pipeline::kMaxColorAttachments> m_colorTargetLocalIndices;
            uint32_t m_depthTargetLocalIndex = kInvalidIndex;

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
        };

        FrameGraph(Core::DescriptorManager* descriptorManager);

        PassNodeBase& AddPassInternal() override;

        void ParsePassPushConstants(PassNode& pass, const RTTI::Type& type);
        void CompilePass(PassNode& pass);

        void Compile();
        void Execute();

        uint32_t RegisterResource(Core::Resource* resource, uint32_t passIndex, uint32_t accessIndex);

        Core::DescriptorManager* m_descriptorManager = nullptr;

        SegmentedVector<PassNode> m_passes;
        SegmentedVector<ResourceNode> m_resources;
        festd::segmented_unordered_dense_map<uint32_t, uint32_t> m_resourceIndexMap;
    };
} // namespace FE::Graphics::Common
