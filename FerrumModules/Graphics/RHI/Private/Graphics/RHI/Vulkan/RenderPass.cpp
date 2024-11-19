#include <Graphics/RHI/Vulkan/CommandList.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/ImageFormat.h>
#include <Graphics/RHI/Vulkan/PipelineStates.h>
#include <Graphics/RHI/Vulkan/RenderPass.h>
#include <Graphics/RHI/Vulkan/ResourceState.h>

namespace FE::Graphics::Vulkan
{
    static VkAttachmentLoadOp VKConvert(RHI::AttachmentLoadOp source)
    {
        switch (source)
        {
        case RHI::AttachmentLoadOp::kDontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case RHI::AttachmentLoadOp::kLoad:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RHI::AttachmentLoadOp::kClear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default:
            FE_AssertMsg(false, "Invalid AttachmentLoadOp");
            return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }
    }


    static VkAttachmentStoreOp VKConvert(RHI::AttachmentStoreOp source)
    {
        switch (source)
        {
        case RHI::AttachmentStoreOp::kDontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case RHI::AttachmentStoreOp::kStore:
            return VK_ATTACHMENT_STORE_OP_STORE;
        default:
            FE_AssertMsg(false, "Invalid AttachmentLoadOp");
            return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }
    }


    RenderPass::RenderPass(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode RenderPass::Init(const RHI::RenderPassDesc& desc)
    {
        m_desc = desc;
        BuildNativeRenderPass();
        return RHI::ResultCode::kSuccess;
    }


    uint32_t RenderPass::GetAttachmentCount()
    {
        return m_desc.m_attachments.size();
    }


    void RenderPass::BuildNativeRenderPass()
    {
        auto attachmentDescriptions = BuildAttachmentDescriptions();

        festd::vector<SubpassAttachmentReferences> subpassAttachmentReferences;
        for (uint32_t i = 0; i < m_desc.m_subpasses.size(); ++i)
        {
            auto& refs = subpassAttachmentReferences.emplace_back();

            auto& depthStencilAttachmentRef = refs.m_depthStencil;
            auto& depthStencilAttachment = m_desc.m_subpasses[i].m_depthStencilAttachment;
            depthStencilAttachmentRef.layout = VKConvert(depthStencilAttachment.m_state);
            depthStencilAttachmentRef.attachment = depthStencilAttachment.m_index;

            refs.m_input = BuildAttachmentReferences(i, RHI::AttachmentType::kInput);
            refs.m_rt = BuildAttachmentReferences(i, RHI::AttachmentType::kRenderTarget);
            refs.m_resolve = BuildAttachmentReferences(i, RHI::AttachmentType::kMSAAResolve);

            const auto& preserve = m_desc.m_subpasses[i].m_preserveAttachments;
            refs.m_preserve = preserve.empty() ? nullptr : preserve.data();
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

        vkCreateRenderPass(NativeCast(m_device), &renderPassCI, VK_NULL_HANDLE, &m_nativeRenderPass);
    }


    festd::vector<VkAttachmentDescription> RenderPass::BuildAttachmentDescriptions()
    {
        festd::vector<VkAttachmentDescription> result;
        result.reserve(GetAttachmentCount());

        for (const auto& attachmentDesc : m_desc.m_attachments)
        {
            auto& nativeDesc = result.push_back();
            nativeDesc.format = VKConvert(attachmentDesc.m_format);
            nativeDesc.loadOp = VKConvert(attachmentDesc.m_loadOp);
            nativeDesc.storeOp = VKConvert(attachmentDesc.m_storeOp);
            nativeDesc.stencilLoadOp = VKConvert(attachmentDesc.m_stencilLoadOp);
            nativeDesc.stencilStoreOp = VKConvert(attachmentDesc.m_stencilStoreOp);
            nativeDesc.initialLayout = VKConvert(attachmentDesc.m_initialState);
            nativeDesc.finalLayout = VKConvert(attachmentDesc.m_finalState);
            nativeDesc.samples = GetVKSampleCountFlags(attachmentDesc.m_sampleCount);
        }

        return result;
    }


    festd::vector<VkSubpassDescription> RenderPass::BuildSubpassDescriptions(
        festd::span<const SubpassAttachmentReferences> subpassAttachmentReferences) const
    {
        festd::vector<VkSubpassDescription> result;
        result.reserve(m_desc.m_subpasses.size());

        for (uint32_t i = 0; i < m_desc.m_subpasses.size(); ++i)
        {
            auto& currentRefs = subpassAttachmentReferences[i];

            auto& nativeDesc = result.push_back();
            nativeDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            nativeDesc.inputAttachmentCount = static_cast<uint32_t>(currentRefs.m_input.size());
            nativeDesc.pInputAttachments = currentRefs.m_input.data();

            nativeDesc.colorAttachmentCount = static_cast<uint32_t>(currentRefs.m_rt.size());
            nativeDesc.pColorAttachments = currentRefs.m_rt.data();
            nativeDesc.pResolveAttachments = currentRefs.m_resolve.data();

            nativeDesc.pDepthStencilAttachment =
                currentRefs.m_depthStencil.attachment == static_cast<uint32_t>(-1) ? nullptr : &currentRefs.m_depthStencil;
            nativeDesc.pPreserveAttachments = currentRefs.m_preserve;
        }

        return result;
    }


    festd::vector<VkAttachmentReference> RenderPass::BuildAttachmentReferences(uint32_t subpassIndex,
                                                                               RHI::AttachmentType attachmentType)
    {
        festd::vector<VkAttachmentReference> result;

        const festd::span<const RHI::SubpassAttachment>* attachments;
        switch (attachmentType)
        {
        case RHI::AttachmentType::kInput:
            attachments = &m_desc.m_subpasses[subpassIndex].m_inputAttachments;
            break;
        case RHI::AttachmentType::kRenderTarget:
            attachments = &m_desc.m_subpasses[subpassIndex].m_renderTargetAttachments;
            break;
        case RHI::AttachmentType::kMSAAResolve:
            attachments = &m_desc.m_subpasses[subpassIndex].m_msaaResolveAttachments;
            break;
        default:
            FE_AssertMsg(false, "Invalid AttachmentType, note that Preserve and DepthStencil attachments are not allowed here");
            return result;
        }

        for (auto& attachment : *attachments)
        {
            auto& ref = result.push_back();
            ref.attachment = attachment.m_index;
            ref.layout = VKConvert(attachment.m_state);
        }

        return result;
    }


    festd::vector<VkSubpassDependency> RenderPass::BuildSubpassDependencies()
    {
        festd::vector<VkSubpassDependency> result;

        static auto validateSubpassIndex = [this](uint32_t index) {
            return index < m_desc.m_subpasses.size() ? index : VK_SUBPASS_EXTERNAL;
        };

        for (auto& dependency : m_desc.m_subpassDependencies)
        {
            auto& nativeDependency = result.push_back();
            nativeDependency.srcSubpass = validateSubpassIndex(dependency.m_sourceSubpassIndex);
            nativeDependency.dstSubpass = validateSubpassIndex(dependency.m_destinationSubpassIndex);

            nativeDependency.srcStageMask = VKConvert(dependency.m_sourcePipelineStage);
            nativeDependency.srcAccessMask = GetAccessMask(dependency.m_sourceState);

            nativeDependency.dstStageMask = VKConvert(dependency.m_destinationPipelineStage);
            nativeDependency.dstAccessMask = GetAccessMask(dependency.m_destinationState);
        }

        return result;
    }


    RenderPass::~RenderPass()
    {
        if (m_nativeRenderPass)
            vkDestroyRenderPass(NativeCast(m_device), m_nativeRenderPass, nullptr);
    }
} // namespace FE::Graphics::Vulkan
