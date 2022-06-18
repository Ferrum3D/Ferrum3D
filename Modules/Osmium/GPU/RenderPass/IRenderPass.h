#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Containers/List.h>
#include <FeCore/RTTI/RTTI.h>
#include <GPU/Pipeline/PipelineStates.h>
#include <GPU/Resource/ResourceState.h>

namespace FE::GPU
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
        AttachmentLoadOp LoadOp          = AttachmentLoadOp::Clear;
        AttachmentStoreOp StoreOp        = AttachmentStoreOp::Store;
        AttachmentLoadOp StencilLoadOp   = AttachmentLoadOp::DontCare;
        AttachmentStoreOp StencilStoreOp = AttachmentStoreOp::DontCare;

        Format Format = Format::None;

        ResourceState InitialState = ResourceState::Undefined;
        ResourceState FinalState   = ResourceState::Undefined;

        FE_STRUCT_RTTI(AttachmentDesc, "0862DB39-25EE-49D5-8762-38AB54D489BC");
    };

    struct SubpassAttachment
    {
        ResourceState State = ResourceState::Undefined;
        UInt32 Index = static_cast<UInt32>(-1);

        SubpassAttachment() = default;

        inline SubpassAttachment(ResourceState state, UInt32 index)
            : Index(index)
            , State(state)
        {
        }

        FE_STRUCT_RTTI(SubpassAttachment, "2C6AFAF9-4105-48CD-9483-C52B99BEED23");
    };

    enum class AttachmentType
    {
        None,
        Input,
        Preserve,
        DepthStencil,
        RenderTarget
    };

    struct SubpassDesc
    {
        List<SubpassAttachment> InputAttachments;
        List<SubpassAttachment> RenderTargetAttachments;
        List<UInt32> PreserveAttachments;
        SubpassAttachment DepthStencilAttachment;

        FE_STRUCT_RTTI(SubpassDesc, "8A770B89-4A7B-43C8-B2C5-1503DF4B547D");
    };

    struct SubpassDependency
    {
        UInt32 SourceSubpassIndex              = static_cast<UInt32>(-1);
        PipelineStageFlags SourcePipelineStage = PipelineStageFlags::ColorAttachmentOutput;
        ResourceState SourceState              = ResourceState::Common;

        UInt32 DestinationSubpassIndex              = 0;
        PipelineStageFlags DestinationPipelineStage = PipelineStageFlags::ColorAttachmentOutput;
        ResourceState DestinationState              = ResourceState::RenderTarget;

        FE_STRUCT_RTTI(SubpassDependency, "0DE679C1-EFF0-4238-AAC4-50BA4D7C5DF6");
    };

    struct RenderPassDesc
    {
        List<SubpassDesc> Subpasses;
        List<AttachmentDesc> Attachments;
        List<SubpassDependency> SubpassDependencies;

        FE_STRUCT_RTTI(RenderPassDesc, "41AC7F3A-6A7C-4A85-8D11-3D6E0B5B70AD");
    };

    class IRenderPass : public IObject
    {
    public:
        FE_CLASS_RTTI(IRenderPass, "CA5F6A54-76F3-4A00-8484-B2F9DEF4B3BB");

        virtual UInt32 GetAttachmentCount() = 0;
    };
} // namespace FE::GPU
