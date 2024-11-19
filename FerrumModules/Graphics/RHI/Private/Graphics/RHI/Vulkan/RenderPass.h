#pragma once
#include <Graphics/RHI/RenderPass.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkPipelineStageFlags VKConvert(RHI::PipelineStageFlags source)
    {
        switch (source)
        {
        case RHI::PipelineStageFlags::kTopOfPipe:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case RHI::PipelineStageFlags::kDrawIndirect:
            return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case RHI::PipelineStageFlags::kVertexInput:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case RHI::PipelineStageFlags::kVertexShader:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case RHI::PipelineStageFlags::kTessellationControlShader:
            return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        case RHI::PipelineStageFlags::kTessellationEvaluationShader:
            return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        case RHI::PipelineStageFlags::kGeometryShader:
            return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        case RHI::PipelineStageFlags::kFragmentShader:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case RHI::PipelineStageFlags::kEarlyFragmentTests:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case RHI::PipelineStageFlags::kLateFragmentTests:
            return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case RHI::PipelineStageFlags::kColorAttachmentOutput:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case RHI::PipelineStageFlags::kComputeShader:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case RHI::PipelineStageFlags::kTransfer:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case RHI::PipelineStageFlags::kBottomOfPipe:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case RHI::PipelineStageFlags::kHost:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case RHI::PipelineStageFlags::kAllGraphics:
            return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        default:
            FE_AssertMsg(false, "Invalid PipelineStageFlags");
            return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }


    struct RenderPass final : public RHI::RenderPass
    {
        FE_RTTI_Class(RenderPass, "091A0BB6-816E-4144-AE03-D082C1C7B689");

        RenderPass(RHI::Device* device);
        ~RenderPass() override;

        RHI::ResultCode Init(const RHI::RenderPassDesc& desc) override;

        uint32_t GetAttachmentCount() override;

        [[nodiscard]] VkRenderPass GetNative() const
        {
            return m_nativeRenderPass;
        }

    private:
        RHI::RenderPassDesc m_desc;

        VkRenderPass m_nativeRenderPass = VK_NULL_HANDLE;

        struct SubpassAttachmentReferences final
        {
            festd::vector<VkAttachmentReference> m_input;
            festd::vector<VkAttachmentReference> m_rt;
            festd::vector<VkAttachmentReference> m_resolve;
            const uint32_t* m_preserve;
            VkAttachmentReference m_depthStencil;
        };

        void BuildNativeRenderPass();
        festd::vector<VkAttachmentDescription> BuildAttachmentDescriptions();
        festd::vector<VkSubpassDescription> BuildSubpassDescriptions(
            festd::span<const SubpassAttachmentReferences> subpassAttachmentReferences) const;
        festd::vector<VkAttachmentReference> BuildAttachmentReferences(uint32_t subpassIndex, RHI::AttachmentType attachmentType);
        festd::vector<VkSubpassDependency> BuildSubpassDependencies();
    };


    FE_ENABLE_NATIVE_CAST(RenderPass);
} // namespace FE::Graphics::Vulkan
