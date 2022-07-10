#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/RenderPass/IRenderPass.h>

namespace FE::Osmium
{
    class VKDevice;

    inline VkPipelineStageFlags VKConvert(PipelineStageFlags source)
    {
        switch (source)
        {
        case PipelineStageFlags::TopOfPipe:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case PipelineStageFlags::DrawIndirect:
            return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case PipelineStageFlags::VertexInput:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case PipelineStageFlags::VertexShader:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case PipelineStageFlags::TessellationControlShader:
            return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        case PipelineStageFlags::TessellationEvaluationShader:
            return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        case PipelineStageFlags::GeometryShader:
            return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        case PipelineStageFlags::FragmentShader:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case PipelineStageFlags::EarlyFragmentTests:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case PipelineStageFlags::LateFragmentTests:
            return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case PipelineStageFlags::ColorAttachmentOutput:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case PipelineStageFlags::ComputeShader:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case PipelineStageFlags::Transfer:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case PipelineStageFlags::BottomOfPipe:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case PipelineStageFlags::Host:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case PipelineStageFlags::AllGraphics:
            return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        default:
            FE_UNREACHABLE("Invalid PipelineStageFlags");
            return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    class VKRenderPass : public Object<IRenderPass>
    {
        VKDevice* m_Device;
        RenderPassDesc m_Desc;

        VkRenderPass m_NativeRenderPass;

        struct SubpassAttachmentReferences
        {
            List<VkAttachmentReference> Input;
            List<VkAttachmentReference> RT;
            List<VkAttachmentReference> Resolve;
            const UInt32* Preserve;
            VkAttachmentReference DepthStencil;
        };

        void BuildNativeRenderPass();
        List<VkAttachmentDescription> BuildAttachmentDescriptions();
        List<VkSubpassDescription> BuildSubpassDescriptions(
            List<VKRenderPass::SubpassAttachmentReferences>& subpassAttachmentReferences) const;
        List<VkAttachmentReference> BuildAttachmentReferences(UInt32 subpassIndex, AttachmentType attachmentType);
        List<VkSubpassDependency> BuildSubpassDependencies();

    public:
        FE_CLASS_RTTI(VKRenderPass, "091A0BB6-816E-4144-AE03-D082C1C7B689");

        VKRenderPass(VKDevice& dev, const RenderPassDesc& desc);
        ~VKRenderPass() override;
        UInt32 GetAttachmentCount() override;

        inline VkRenderPass& GetNativeRenderPass();
    };

    inline VkRenderPass& VKRenderPass::GetNativeRenderPass()
    {
        return m_NativeRenderPass;
    }
} // namespace FE::Osmium
