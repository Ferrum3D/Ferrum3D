#include <OsGPU/CommandBuffer/VKCommandBuffer.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/Pipeline/VKPipelineStates.h>
#include <OsGPU/RenderPass/VKRenderPass.h>
#include <OsGPU/Resource/VKResourceState.h>

namespace FE::Osmium
{
    static VkAttachmentLoadOp VKConvert(AttachmentLoadOp source)
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

    static VkAttachmentStoreOp VKConvert(AttachmentStoreOp source)
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

    uint32_t VKRenderPass::GetAttachmentCount()
    {
        return static_cast<uint32_t>(m_Desc.Attachments.Length());
    }

    void VKRenderPass::BuildNativeRenderPass()
    {
        auto attachmentDescriptions = BuildAttachmentDescriptions();

        eastl::vector<SubpassAttachmentReferences> subpassAttachmentReferences;
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_Desc.Subpasses.Length()); ++i)
        {
            auto& refs = subpassAttachmentReferences.push_back();

            auto& depthStencilAttachmentRef = refs.DepthStencil;
            auto& depthStencilAttachment = m_Desc.Subpasses[i].DepthStencilAttachment;
            depthStencilAttachmentRef.layout = VKConvert(depthStencilAttachment.State);
            depthStencilAttachmentRef.attachment = depthStencilAttachment.Index;

            refs.Input = BuildAttachmentReferences(i, AttachmentType::Input);
            refs.RT = BuildAttachmentReferences(i, AttachmentType::RenderTarget);
            refs.Resolve = BuildAttachmentReferences(i, AttachmentType::MSAAResolve);

            auto& preserve = m_Desc.Subpasses[i].PreserveAttachments;
            refs.Preserve = preserve.Empty() ? nullptr : preserve.Data();
        }

        auto subpassDescriptions = BuildSubpassDescriptions(subpassAttachmentReferences);
        auto subpassDependencies = BuildSubpassDependencies();

        VkRenderPassCreateInfo renderPassCI{};
        renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassCI.pAttachments = attachmentDescriptions.data();

        renderPassCI.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderPassCI.pDependencies = subpassDependencies.data();

        renderPassCI.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
        renderPassCI.pSubpasses = subpassDescriptions.data();

        vkCreateRenderPass(m_Device->GetNativeDevice(), &renderPassCI, VK_NULL_HANDLE, &m_NativeRenderPass);
    }

    eastl::vector<VkAttachmentDescription> VKRenderPass::BuildAttachmentDescriptions()
    {
        eastl::vector<VkAttachmentDescription> result;
        result.reserve(GetAttachmentCount());

        for (const auto& attachmentDesc : m_Desc.Attachments)
        {
            auto& nativeDesc = result.push_back();
            nativeDesc.format = VKConvert(attachmentDesc.Format);
            nativeDesc.loadOp = VKConvert(attachmentDesc.LoadOp);
            nativeDesc.storeOp = VKConvert(attachmentDesc.StoreOp);
            nativeDesc.stencilLoadOp = VKConvert(attachmentDesc.StencilLoadOp);
            nativeDesc.stencilStoreOp = VKConvert(attachmentDesc.StencilStoreOp);
            nativeDesc.initialLayout = VKConvert(attachmentDesc.InitialState);
            nativeDesc.finalLayout = VKConvert(attachmentDesc.FinalState);
            nativeDesc.samples = GetVKSampleCountFlags(attachmentDesc.SampleCount);
        }

        return result;
    }

    eastl::vector<VkSubpassDescription> VKRenderPass::BuildSubpassDescriptions(
        eastl::vector<VKRenderPass::SubpassAttachmentReferences>& subpassAttachmentReferences) const
    {
        eastl::vector<VkSubpassDescription> result;
        result.reserve(m_Desc.Subpasses.Length());

        for (uint32_t i = 0; i < m_Desc.Subpasses.Length(); ++i)
        {
            auto& currentRefs = subpassAttachmentReferences[i];

            auto& nativeDesc = result.push_back();
            nativeDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            nativeDesc.inputAttachmentCount = static_cast<uint32_t>(currentRefs.Input.size());
            nativeDesc.pInputAttachments = currentRefs.Input.data();

            nativeDesc.colorAttachmentCount = static_cast<uint32_t>(currentRefs.RT.size());
            nativeDesc.pColorAttachments = currentRefs.RT.data();
            nativeDesc.pResolveAttachments = currentRefs.Resolve.data();

            nativeDesc.pDepthStencilAttachment =
                currentRefs.DepthStencil.attachment == static_cast<uint32_t>(-1) ? nullptr : &currentRefs.DepthStencil;
            nativeDesc.pPreserveAttachments = currentRefs.Preserve;
        }

        return result;
    }

    eastl::vector<VkAttachmentReference> VKRenderPass::BuildAttachmentReferences(uint32_t subpassIndex,
                                                                                 AttachmentType attachmentType)
    {
        eastl::vector<VkAttachmentReference> result;

        const ArraySlice<SubpassAttachment>* attachments;
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
            auto& ref = result.push_back();
            ref.attachment = attachment.Index;
            ref.layout = VKConvert(attachment.State);
        }

        return result;
    }

    eastl::vector<VkSubpassDependency> VKRenderPass::BuildSubpassDependencies()
    {
        eastl::vector<VkSubpassDependency> result;

        static auto validateSubpassIndex = [this](uint32_t index) {
            return index < m_Desc.Subpasses.Length() ? index : VK_SUBPASS_EXTERNAL;
        };

        for (auto& dependency : m_Desc.SubpassDependencies)
        {
            auto& nativeDependency = result.push_back();
            nativeDependency.srcSubpass = validateSubpassIndex(dependency.SourceSubpassIndex);
            nativeDependency.dstSubpass = validateSubpassIndex(dependency.DestinationSubpassIndex);

            nativeDependency.srcStageMask = VKConvert(dependency.SourcePipelineStage);
            nativeDependency.srcAccessMask = GetAccessMask(dependency.SourceState);

            nativeDependency.dstStageMask = VKConvert(dependency.DestinationPipelineStage);
            nativeDependency.dstAccessMask = GetAccessMask(dependency.DestinationState);
        }

        return result;
    }

    FE_VK_OBJECT_DELETER(RenderPass);

    VKRenderPass::~VKRenderPass()
    {
        FE_DELETE_VK_OBJECT(RenderPass, m_NativeRenderPass);
    }
} // namespace FE::Osmium
