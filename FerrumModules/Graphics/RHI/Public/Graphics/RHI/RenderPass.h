#pragma once
#include <FeCore/Memory/Memory.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/PipelineStates.h>
#include <Graphics/RHI/ResourceState.h>

namespace FE::Graphics::RHI
{
    enum class AttachmentLoadOp : uint32_t
    {
        kDontCare,
        kLoad,
        kClear
    };


    enum class AttachmentStoreOp : uint32_t
    {
        kDontCare,
        kStore
    };


    struct AttachmentDesc final
    {
        AttachmentLoadOp m_loadOp = AttachmentLoadOp::kClear;
        AttachmentStoreOp m_storeOp = AttachmentStoreOp::kStore;
        AttachmentLoadOp m_stencilLoadOp = AttachmentLoadOp::kDontCare;
        AttachmentStoreOp m_stencilStoreOp = AttachmentStoreOp::kDontCare;

        Format m_format = Format::kUndefined;

        ResourceState m_initialState = ResourceState::kUndefined;
        ResourceState m_finalState = ResourceState::kUndefined;

        int32_t m_sampleCount = 1;
    };


    struct SubpassAttachment final
    {
        ResourceState m_state = ResourceState::kUndefined;
        uint32_t m_index = UINT32_MAX;

        SubpassAttachment() = default;

        SubpassAttachment(ResourceState state, uint32_t index)
            : m_index(index)
            , m_state(state)
        {
        }
    };


    enum class AttachmentType : uint32_t
    {
        kNone,
        kInput,
        kPreserve,
        kDepthStencil,
        kRenderTarget,
        kMSAAResolve
    };


    struct SubpassDesc final
    {
        festd::span<const SubpassAttachment> m_inputAttachments;
        festd::span<const SubpassAttachment> m_renderTargetAttachments;
        festd::span<const SubpassAttachment> m_msaaResolveAttachments;
        festd::span<const uint32_t> m_preserveAttachments;
        SubpassAttachment m_depthStencilAttachment;
    };


    struct SubpassDependency final
    {
        uint32_t m_sourceSubpassIndex = UINT32_MAX;
        PipelineStageFlags m_sourcePipelineStage = PipelineStageFlags::kColorAttachmentOutput;
        ResourceState m_sourceState = ResourceState::kCommon;

        uint32_t m_destinationSubpassIndex = 0;
        PipelineStageFlags m_destinationPipelineStage = PipelineStageFlags::kColorAttachmentOutput;
        ResourceState m_destinationState = ResourceState::kRenderTarget;
    };


    struct RenderPassDesc final
    {
        festd::span<const SubpassDesc> m_subpasses;
        festd::span<const AttachmentDesc> m_attachments;
        festd::span<const SubpassDependency> m_subpassDependencies;
    };


    struct RenderPass : public DeviceObject
    {
        FE_RTTI_Class(RenderPass, "CA5F6A54-76F3-4A00-8484-B2F9DEF4B3BB");

        ~RenderPass() override = default;

        virtual ResultCode Init(const RenderPassDesc& desc) = 0;

        virtual uint32_t GetAttachmentCount() = 0;
    };
} // namespace FE::Graphics::RHI
