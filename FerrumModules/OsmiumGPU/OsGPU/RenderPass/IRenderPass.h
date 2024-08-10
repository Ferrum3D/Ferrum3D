#pragma once
#include <FeCore/Memory/Memory.h>
#include <OsGPU/Pipeline/PipelineStates.h>
#include <OsGPU/Resource/ResourceState.h>

namespace FE::Osmium
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

        Format Format = Format::None;

        ResourceState InitialState = ResourceState::Undefined;
        ResourceState FinalState = ResourceState::Undefined;

        int32_t SampleCount = 1;

        FE_RTTI_Base(AttachmentDesc, "0862DB39-25EE-49D5-8762-38AB54D489BC");
    };

    struct SubpassAttachment
    {
        ResourceState State = ResourceState::Undefined;
        uint32_t Index = static_cast<uint32_t>(-1);

        SubpassAttachment() = default;

        inline SubpassAttachment(ResourceState state, uint32_t index)
            : Index(index)
            , State(state)
        {
        }

        FE_RTTI_Base(SubpassAttachment, "2C6AFAF9-4105-48CD-9483-C52B99BEED23");
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
        ArraySlice<SubpassAttachment> InputAttachments;
        ArraySlice<SubpassAttachment> RenderTargetAttachments;
        ArraySlice<SubpassAttachment> MSAAResolveAttachments;
        ArraySlice<uint32_t> PreserveAttachments;
        SubpassAttachment DepthStencilAttachment;

        FE_RTTI_Base(SubpassDesc, "8A770B89-4A7B-43C8-B2C5-1503DF4B547D");
    };

    struct SubpassDependency
    {
        uint32_t SourceSubpassIndex = static_cast<uint32_t>(-1);
        PipelineStageFlags SourcePipelineStage = PipelineStageFlags::ColorAttachmentOutput;
        ResourceState SourceState = ResourceState::Common;

        uint32_t DestinationSubpassIndex = 0;
        PipelineStageFlags DestinationPipelineStage = PipelineStageFlags::ColorAttachmentOutput;
        ResourceState DestinationState = ResourceState::RenderTarget;

        FE_RTTI_Base(SubpassDependency, "0DE679C1-EFF0-4238-AAC4-50BA4D7C5DF6");
    };

    struct RenderPassDesc
    {
        ArraySlice<SubpassDesc> Subpasses;
        ArraySlice<AttachmentDesc> Attachments;
        ArraySlice<SubpassDependency> SubpassDependencies;

        FE_RTTI_Base(RenderPassDesc, "41AC7F3A-6A7C-4A85-8D11-3D6E0B5B70AD");
    };

    class IRenderPass : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IRenderPass, "CA5F6A54-76F3-4A00-8484-B2F9DEF4B3BB");

        virtual uint32_t GetAttachmentCount() = 0;
    };
} // namespace FE::Osmium
