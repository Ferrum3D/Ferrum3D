#include <HAL/Vulkan/CommandList.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/ImageFormat.h>
#include <HAL/Vulkan/PipelineStates.h>
#include <HAL/Vulkan/RenderPass.h>
#include <HAL/Vulkan/ResourceState.h>

namespace FE::Graphics::Vulkan
{
    static VkAttachmentLoadOp VKConvert(HAL::AttachmentLoadOp source)
    {
        switch (source)
        {
        case HAL::AttachmentLoadOp::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case HAL::AttachmentLoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case HAL::AttachmentLoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default:
            FE_AssertMsg(false, "Invalid AttachmentLoadOp");
            return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }
    }


    static VkAttachmentStoreOp VKConvert(HAL::AttachmentStoreOp source)
    {
        switch (source)
        {
        case HAL::AttachmentStoreOp::DontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case HAL::AttachmentStoreOp::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        default:
            FE_AssertMsg(false, "Invalid AttachmentLoadOp");
            return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }
    }


    RenderPass::RenderPass(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode RenderPass::Init(const HAL::RenderPassDesc& desc)
    {
        m_Desc = desc;
        BuildNativeRenderPass();
        return HAL::ResultCode::Success;
    }


    uint32_t RenderPass::GetAttachmentCount()
    {
        return m_Desc.Attachments.size();
    }


    void RenderPass::BuildNativeRenderPass()
    {
        auto attachmentDescriptions = BuildAttachmentDescriptions();

        festd::vector<SubpassAttachmentReferences> subpassAttachmentReferences;
        for (uint32_t i = 0; i < m_Desc.Subpasses.size(); ++i)
        {
            auto& refs = subpassAttachmentReferences.emplace_back();

            auto& depthStencilAttachmentRef = refs.DepthStencil;
            auto& depthStencilAttachment = m_Desc.Subpasses[i].DepthStencilAttachment;
            depthStencilAttachmentRef.layout = VKConvert(depthStencilAttachment.State);
            depthStencilAttachmentRef.attachment = depthStencilAttachment.Index;

            refs.Input = BuildAttachmentReferences(i, HAL::AttachmentType::Input);
            refs.RT = BuildAttachmentReferences(i, HAL::AttachmentType::RenderTarget);
            refs.Resolve = BuildAttachmentReferences(i, HAL::AttachmentType::MSAAResolve);

            auto& preserve = m_Desc.Subpasses[i].PreserveAttachments;
            refs.Preserve = preserve.empty() ? nullptr : preserve.data();
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

        vkCreateRenderPass(NativeCast(m_pDevice), &renderPassCI, VK_NULL_HANDLE, &m_NativeRenderPass);
    }


    eastl::vector<VkAttachmentDescription> RenderPass::BuildAttachmentDescriptions()
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


    eastl::vector<VkSubpassDescription> RenderPass::BuildSubpassDescriptions(
        festd::span<const SubpassAttachmentReferences> subpassAttachmentReferences) const
    {
        eastl::vector<VkSubpassDescription> result;
        result.reserve(m_Desc.Subpasses.size());

        for (uint32_t i = 0; i < m_Desc.Subpasses.size(); ++i)
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


    eastl::vector<VkAttachmentReference> RenderPass::BuildAttachmentReferences(uint32_t subpassIndex,
                                                                               HAL::AttachmentType attachmentType)
    {
        eastl::vector<VkAttachmentReference> result;

        const festd::span<const HAL::SubpassAttachment>* attachments;
        switch (attachmentType)
        {
        case HAL::AttachmentType::Input:
            attachments = &m_Desc.Subpasses[subpassIndex].InputAttachments;
            break;
        case HAL::AttachmentType::RenderTarget:
            attachments = &m_Desc.Subpasses[subpassIndex].RenderTargetAttachments;
            break;
        case HAL::AttachmentType::MSAAResolve:
            attachments = &m_Desc.Subpasses[subpassIndex].MSAAResolveAttachments;
            break;
        default:
            FE_AssertMsg(false, "Invalid AttachmentType, note that Preserve and DepthStencil attachments are not allowed here");
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


    eastl::vector<VkSubpassDependency> RenderPass::BuildSubpassDependencies()
    {
        eastl::vector<VkSubpassDependency> result;

        static auto validateSubpassIndex = [this](uint32_t index) {
            return index < m_Desc.Subpasses.size() ? index : VK_SUBPASS_EXTERNAL;
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


    RenderPass::~RenderPass()
    {
        if (m_NativeRenderPass)
            vkDestroyRenderPass(NativeCast(m_pDevice), m_NativeRenderPass, nullptr);
    }
} // namespace FE::Graphics::Vulkan
