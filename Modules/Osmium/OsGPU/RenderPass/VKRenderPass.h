#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/RenderPass/IRenderPass.h>

namespace FE::GPU
{
    class VKDevice;

    inline vk::PipelineStageFlags VKConvert(PipelineStageFlags source)
    {
        switch (source)
        {
        case PipelineStageFlags::TopOfPipe:
            return vk::PipelineStageFlagBits::eTopOfPipe;
        case PipelineStageFlags::DrawIndirect:
            return vk::PipelineStageFlagBits::eDrawIndirect;
        case PipelineStageFlags::VertexInput:
            return vk::PipelineStageFlagBits::eVertexInput;
        case PipelineStageFlags::VertexShader:
            return vk::PipelineStageFlagBits::eVertexShader;
        case PipelineStageFlags::TessellationControlShader:
            return vk::PipelineStageFlagBits::eTessellationControlShader;
        case PipelineStageFlags::TessellationEvaluationShader:
            return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
        case PipelineStageFlags::GeometryShader:
            return vk::PipelineStageFlagBits::eGeometryShader;
        case PipelineStageFlags::FragmentShader:
            return vk::PipelineStageFlagBits::eFragmentShader;
        case PipelineStageFlags::EarlyFragmentTests:
            return vk::PipelineStageFlagBits::eEarlyFragmentTests;
        case PipelineStageFlags::LateFragmentTests:
            return vk::PipelineStageFlagBits::eLateFragmentTests;
        case PipelineStageFlags::ColorAttachmentOutput:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;
        case PipelineStageFlags::ComputeShader:
            return vk::PipelineStageFlagBits::eComputeShader;
        case PipelineStageFlags::Transfer:
            return vk::PipelineStageFlagBits::eTransfer;
        case PipelineStageFlags::BottomOfPipe:
            return vk::PipelineStageFlagBits::eBottomOfPipe;
        case PipelineStageFlags::Host:
            return vk::PipelineStageFlagBits::eHost;
        case PipelineStageFlags::AllGraphics:
            return vk::PipelineStageFlagBits::eAllGraphics;
        default:
            FE_UNREACHABLE("Invalid PipelineStageFlags");
            return static_cast<vk::PipelineStageFlagBits>(-1);
        }
    }

    class VKRenderPass : public Object<IRenderPass>
    {
        VKDevice* m_Device;
        RenderPassDesc m_Desc;

        vk::UniqueRenderPass m_NativeRenderPass;

        struct SubpassAttachmentReferences
        {
            Vector<vk::AttachmentReference> Input;
            Vector<vk::AttachmentReference> RT;
            UInt32* Preserve;
            vk::AttachmentReference DepthStencil;
        };

        void BuildNativeRenderPass();
        Vector<vk::AttachmentDescription> BuildAttachmentDescriptions();
        Vector<vk::SubpassDescription> BuildSubpassDescriptions(
            Vector<VKRenderPass::SubpassAttachmentReferences>& subpassAttachmentReferences) const;
        Vector<vk::AttachmentReference> BuildAttachmentReferences(UInt32 subpassIndex, AttachmentType attachmentType);
        Vector<vk::SubpassDependency> BuildSubpassDependencies();

    public:
        FE_CLASS_RTTI(VKRenderPass, "091A0BB6-816E-4144-AE03-D082C1C7B689");

        VKRenderPass(VKDevice& dev, const RenderPassDesc& desc);
        UInt32 GetAttachmentCount() override;

        inline vk::RenderPass& GetNativeRenderPass();
    };

    inline vk::RenderPass& VKRenderPass::GetNativeRenderPass()
    {
        return m_NativeRenderPass.get();
    }
} // namespace FE::GPU
