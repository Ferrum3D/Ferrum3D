#pragma once
#include <HAL/RenderPass.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkPipelineStageFlags VKConvert(HAL::PipelineStageFlags source)
    {
        switch (source)
        {
        case HAL::PipelineStageFlags::kTopOfPipe:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case HAL::PipelineStageFlags::kDrawIndirect:
            return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case HAL::PipelineStageFlags::kVertexInput:
            return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        case HAL::PipelineStageFlags::kVertexShader:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        case HAL::PipelineStageFlags::kTessellationControlShader:
            return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        case HAL::PipelineStageFlags::kTessellationEvaluationShader:
            return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        case HAL::PipelineStageFlags::kGeometryShader:
            return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        case HAL::PipelineStageFlags::kFragmentShader:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case HAL::PipelineStageFlags::kEarlyFragmentTests:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case HAL::PipelineStageFlags::kLateFragmentTests:
            return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case HAL::PipelineStageFlags::kColorAttachmentOutput:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case HAL::PipelineStageFlags::kComputeShader:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case HAL::PipelineStageFlags::kTransfer:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case HAL::PipelineStageFlags::kBottomOfPipe:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case HAL::PipelineStageFlags::kHost:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case HAL::PipelineStageFlags::kAllGraphics:
            return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        default:
            FE_AssertMsg(false, "Invalid PipelineStageFlags");
            return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    class RenderPass final : public HAL::RenderPass
    {
        HAL::RenderPassDesc m_Desc;

        VkRenderPass m_NativeRenderPass = VK_NULL_HANDLE;

        struct SubpassAttachmentReferences
        {
            festd::vector<VkAttachmentReference> Input;
            festd::vector<VkAttachmentReference> RT;
            festd::vector<VkAttachmentReference> Resolve;
            const uint32_t* Preserve;
            VkAttachmentReference DepthStencil;
        };

        void BuildNativeRenderPass();
        festd::vector<VkAttachmentDescription> BuildAttachmentDescriptions();
        festd::vector<VkSubpassDescription> BuildSubpassDescriptions(
            festd::span<const SubpassAttachmentReferences> subpassAttachmentReferences) const;
        festd::vector<VkAttachmentReference> BuildAttachmentReferences(uint32_t subpassIndex, HAL::AttachmentType attachmentType);
        festd::vector<VkSubpassDependency> BuildSubpassDependencies();

    public:
        FE_RTTI_Class(RenderPass, "091A0BB6-816E-4144-AE03-D082C1C7B689");

        RenderPass(HAL::Device* pDevice);
        ~RenderPass() override;

        HAL::ResultCode Init(const HAL::RenderPassDesc& desc) override;

        uint32_t GetAttachmentCount() override;

        [[nodiscard]] inline VkRenderPass GetNative() const
        {
            return m_NativeRenderPass;
        }
    };


    FE_ENABLE_NATIVE_CAST(RenderPass);
} // namespace FE::Graphics::Vulkan
