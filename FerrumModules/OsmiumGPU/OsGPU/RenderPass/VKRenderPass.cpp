#include <OsGPU/CommandBuffer/VKCommandBuffer.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/Pipeline/VKPipelineStates.h>
#include <OsGPU/RenderPass/VKRenderPass.h>
#include <OsGPU/Resource/VKResourceState.h>

namespace FE::Osmium
{
    VkAttachmentLoadOp VKConvert(AttachmentLoadOp source)
    {
        switch (source)
        {
        case AttachmentLoadOp::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case AttachmentLoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case AttachmentLoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default:
            FE_UNREACHABLE("Invalid AttachmentLoadOp");
            return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }
    }

    VkAttachmentStoreOp VKConvert(AttachmentStoreOp source)
    {
        switch (source)
        {
        case AttachmentStoreOp::DontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case AttachmentStoreOp::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        default:
            FE_UNREACHABLE("Invalid AttachmentLoadOp");
            return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }
    }

    VKRenderPass::VKRenderPass(VKDevice& dev, const RenderPassDesc& desc) // NOLINT(modernize-pass-by-value)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        BuildNativeRenderPass();
    }

    UInt32 VKRenderPass::GetAttachmentCount()
    {
        return static_cast<UInt32>(m_Desc.Attachments.Size());
    }

    void VKRenderPass::BuildNativeRenderPass()
    {
        auto attachmentDescriptions = BuildAttachmentDescriptions();

        List<SubpassAttachmentReferences> subpassAttachmentReferences;
        for (UInt32 i = 0; i < static_cast<UInt32>(m_Desc.Subpasses.Size()); ++i)
        {
            auto& refs = subpassAttachmentReferences.Emplace();

            auto& depthStencilAttachmentRef      = refs.DepthStencil;
            auto& depthStencilAttachment         = m_Desc.Subpasses[i].DepthStencilAttachment;
            depthStencilAttachmentRef.layout     = VKConvert(depthStencilAttachment.State);
            depthStencilAttachmentRef.attachment = depthStencilAttachment.Index;

            refs.Input   = BuildAttachmentReferences(i, AttachmentType::Input);
            refs.RT      = BuildAttachmentReferences(i, AttachmentType::RenderTarget);
            refs.Resolve = BuildAttachmentReferences(i, AttachmentType::MSAAResolve);

            auto& preserve = m_Desc.Subpasses[i].PreserveAttachments;
            refs.Preserve  = preserve.Empty() ? nullptr : preserve.Data();
        }

        auto subpassDescriptions = BuildSubpassDescriptions(subpassAttachmentReferences);
        auto subpassDependencies = BuildSubpassDependencies();

        VkRenderPassCreateInfo renderPassCI{};
        renderPassCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.attachmentCount = static_cast<UInt32>(attachmentDescriptions.Size());
        renderPassCI.pAttachments    = attachmentDescriptions.Data();

        renderPassCI.dependencyCount = static_cast<UInt32>(subpassDependencies.Size());
        renderPassCI.pDependencies   = subpassDependencies.Data();

        renderPassCI.subpassCount = static_cast<UInt32>(subpassDescriptions.Size());
        renderPassCI.pSubpasses   = subpassDescriptions.Data();

        vkCreateRenderPass(m_Device->GetNativeDevice(), &renderPassCI, VK_NULL_HANDLE, &m_NativeRenderPass);
    }

    List<VkAttachmentDescription> VKRenderPass::BuildAttachmentDescriptions()
    {
        List<VkAttachmentDescription> result;
        result.Reserve(GetAttachmentCount());

        for (const auto& attachmentDesc : m_Desc.Attachments)
        {
            auto& nativeDesc          = result.Emplace();
            nativeDesc.format         = VKConvert(attachmentDesc.Format);
            nativeDesc.loadOp         = VKConvert(attachmentDesc.LoadOp);
            nativeDesc.storeOp        = VKConvert(attachmentDesc.StoreOp);
            nativeDesc.stencilLoadOp  = VKConvert(attachmentDesc.StencilLoadOp);
            nativeDesc.stencilStoreOp = VKConvert(attachmentDesc.StencilStoreOp);
            nativeDesc.initialLayout  = VKConvert(attachmentDesc.InitialState);
            nativeDesc.finalLayout    = VKConvert(attachmentDesc.FinalState);
            nativeDesc.samples        = GetVKSampleCountFlags(attachmentDesc.SampleCount);
        }

        return result;
    }

    List<VkSubpassDescription> VKRenderPass::BuildSubpassDescriptions(
        List<VKRenderPass::SubpassAttachmentReferences>& subpassAttachmentReferences) const
    {
        List<VkSubpassDescription> result;
        result.Reserve(m_Desc.Subpasses.Size());

        for (size_t i = 0; i < m_Desc.Subpasses.Size(); ++i)
        {
            auto& currentRefs = subpassAttachmentReferences[i];

            auto& nativeDesc             = result.Emplace();
            nativeDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            nativeDesc.inputAttachmentCount = static_cast<UInt32>(currentRefs.Input.Size());
            nativeDesc.pInputAttachments    = currentRefs.Input.Data();

            nativeDesc.colorAttachmentCount = static_cast<UInt32>(currentRefs.RT.Size());
            nativeDesc.pColorAttachments    = currentRefs.RT.Data();
            nativeDesc.pResolveAttachments  = currentRefs.Resolve.Data();

            nativeDesc.pDepthStencilAttachment =
                currentRefs.DepthStencil.attachment == static_cast<UInt32>(-1) ? nullptr : &currentRefs.DepthStencil;
            nativeDesc.pPreserveAttachments = currentRefs.Preserve;
        }

        return result;
    }

    List<VkAttachmentReference> VKRenderPass::BuildAttachmentReferences(UInt32 subpassIndex, AttachmentType attachmentType)
    {
        List<VkAttachmentReference> result;

        List<SubpassAttachment>* attachments;
        switch (attachmentType)
        {
        case AttachmentType::Input:
            attachments = &m_Desc.Subpasses[subpassIndex].InputAttachments;
            break;
        case AttachmentType::RenderTarget:
            attachments = &m_Desc.Subpasses[subpassIndex].RenderTargetAttachments;
            break;
        case AttachmentType::MSAAResolve:
            attachments = &m_Desc.Subpasses[subpassIndex].MSAAResolveAttachments;
            break;
        default:
            FE_UNREACHABLE("Invalid AttachmentType, note that Preserve and DepthStencil attachments are not allowed here");
            return result;
        }

        for (auto& attachment : *attachments)
        {
            auto& ref      = result.Emplace();
            ref.attachment = attachment.Index;
            ref.layout     = VKConvert(attachment.State);
        }

        return result;
    }

    List<VkSubpassDependency> VKRenderPass::BuildSubpassDependencies()
    {
        List<VkSubpassDependency> result;

        static auto validateSubpassIndex = [this](UInt32 index) {
            return index < m_Desc.Subpasses.Size() ? index : VK_SUBPASS_EXTERNAL;
        };

        for (auto& dependency : m_Desc.SubpassDependencies)
        {
            auto& nativeDependency      = result.Emplace();
            nativeDependency.srcSubpass = validateSubpassIndex(dependency.SourceSubpassIndex);
            nativeDependency.dstSubpass = validateSubpassIndex(dependency.DestinationSubpassIndex);

            nativeDependency.srcStageMask  = VKConvert(dependency.SourcePipelineStage);
            nativeDependency.srcAccessMask = GetAccessMask(dependency.SourceState);

            nativeDependency.dstStageMask  = VKConvert(dependency.DestinationPipelineStage);
            nativeDependency.dstAccessMask = GetAccessMask(dependency.DestinationState);
        }

        return result;
    }

    VKRenderPass::~VKRenderPass()
    {
        vkDestroyRenderPass(m_Device->GetNativeDevice(), m_NativeRenderPass, VK_NULL_HANDLE);
    }
} // namespace FE::Osmium
