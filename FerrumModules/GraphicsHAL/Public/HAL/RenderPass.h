#pragma once
#include <FeCore/Memory/Memory.h>
#include <HAL/DeviceObject.h>
#include <HAL/PipelineStates.h>
#include <HAL/ResourceState.h>

namespace FE::Graphics::HAL
{
    enum class AttachmentLoadOp
    {
        DontCare,
        Load,
        Clear
    };


    enum class AttachmentStoreOp
    {
        DontCare,
        Store
    };


    struct AttachmentDesc
    {
        AttachmentLoadOp LoadOp = AttachmentLoadOp::Clear;
        AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
        AttachmentLoadOp StencilLoadOp = AttachmentLoadOp::DontCare;
        AttachmentStoreOp StencilStoreOp = AttachmentStoreOp::DontCare;

        Format Format = Format::kUndefined;

        ResourceState InitialState = ResourceState::kUndefined;
        ResourceState FinalState = ResourceState::kUndefined;

        int32_t SampleCount = 1;
    };


    struct SubpassAttachment
    {
        ResourceState State = ResourceState::kUndefined;
        uint32_t Index = static_cast<uint32_t>(-1);

        SubpassAttachment() = default;

        inline SubpassAttachment(ResourceState state, uint32_t index)
            : Index(index)
            , State(state)
        {
        }
    };


    enum class AttachmentType
    {
        None,
        Input,
        Preserve,
        DepthStencil,
        RenderTarget,
        MSAAResolve
    };


    struct SubpassDesc
    {
        festd::span<const SubpassAttachment> InputAttachments;
        festd::span<const SubpassAttachment> RenderTargetAttachments;
        festd::span<const SubpassAttachment> MSAAResolveAttachments;
        festd::span<const uint32_t> PreserveAttachments;
        SubpassAttachment DepthStencilAttachment;
    };


    struct SubpassDependency
    {
        uint32_t SourceSubpassIndex = static_cast<uint32_t>(-1);
        PipelineStageFlags SourcePipelineStage = PipelineStageFlags::kColorAttachmentOutput;
        ResourceState SourceState = ResourceState::kCommon;

        uint32_t DestinationSubpassIndex = 0;
        PipelineStageFlags DestinationPipelineStage = PipelineStageFlags::kColorAttachmentOutput;
        ResourceState DestinationState = ResourceState::kRenderTarget;
    };


    struct RenderPassDesc
    {
        festd::span<const SubpassDesc> Subpasses;
        festd::span<const AttachmentDesc> Attachments;
        festd::span<const SubpassDependency> SubpassDependencies;
    };


    class RenderPass : public DeviceObject
    {
    public:
        FE_RTTI_Class(RenderPass, "CA5F6A54-76F3-4A00-8484-B2F9DEF4B3BB");

        ~RenderPass() override = default;

        virtual ResultCode Init(const RenderPassDesc& desc) = 0;

        virtual uint32_t GetAttachmentCount() = 0;
    };
} // namespace FE::Graphics::HAL
