#include <GPU/CommandBuffer/VKCommandBuffer.h>
#include <GPU/Device/VKDevice.h>
#include <GPU/Image/VKImageFormat.h>
#include <GPU/RenderPass/VKRenderPass.h>

namespace FE::GPU
{
    vk::AttachmentLoadOp VKConvert(AttachmentLoadOp source)
    {
        switch (source)
        {
        case AttachmentLoadOp::DontCare:
            return vk::AttachmentLoadOp::eDontCare;
        case AttachmentLoadOp::Load:
            return vk::AttachmentLoadOp::eLoad;
        case AttachmentLoadOp::Clear:
            return vk::AttachmentLoadOp::eClear;
        default:
            FE_UNREACHABLE("Invalid AttachmentLoadOp");
            return static_cast<vk::AttachmentLoadOp>(-1);
        }
    }

    vk::AttachmentStoreOp VKConvert(AttachmentStoreOp source)
    {
        switch (source)
        {
        case AttachmentStoreOp::DontCare:
            return vk::AttachmentStoreOp::eDontCare;
        case AttachmentStoreOp::Store:
            return vk::AttachmentStoreOp::eStore;
        default:
            FE_UNREACHABLE("Invalid AttachmentLoadOp");
            return static_cast<vk::AttachmentStoreOp>(-1);
        }
    }

    VKRenderPass::VKRenderPass(VKDevice& dev, const RenderPassDesc& desc)
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

        Vector<SubpassAttachmentReferences> subpassAttachmentReferences;
        for (UInt32 i = 0; i < static_cast<UInt32>(m_Desc.Subpasses.Size()); ++i)
        {
            auto& refs = subpassAttachmentReferences.emplace_back();

            auto& depthStencilAttachmentRef      = refs.DepthStencil;
            auto& depthStencilAttachment         = m_Desc.Subpasses[i].DepthStencilAttachment;
            depthStencilAttachmentRef.layout     = VKConvert(depthStencilAttachment.State);
            depthStencilAttachmentRef.attachment = depthStencilAttachment.Index;

            refs.Input = BuildAttachmentReferences(i, AttachmentType::Input);
            refs.RT    = BuildAttachmentReferences(i, AttachmentType::RenderTarget);

            auto& preserve = m_Desc.Subpasses[i].PreserveAttachments;
            refs.Preserve  = preserve.Empty() ? nullptr : preserve.Data();
        }

        auto subpassDescriptions = BuildSubpassDescriptions(subpassAttachmentReferences);
        auto subpassDependencies = BuildSubpassDependencies();

        vk::RenderPassCreateInfo renderPassCI{};
        renderPassCI.attachmentCount = static_cast<UInt32>(attachmentDescriptions.size());
        renderPassCI.pAttachments    = attachmentDescriptions.data();

        renderPassCI.dependencyCount = static_cast<UInt32>(subpassDependencies.size());
        renderPassCI.pDependencies   = subpassDependencies.data();

        renderPassCI.subpassCount = static_cast<UInt32>(subpassDescriptions.size());
        renderPassCI.pSubpasses   = subpassDescriptions.data();

        m_NativeRenderPass = m_Device->GetNativeDevice().createRenderPassUnique(renderPassCI);
    }

    Vector<vk::AttachmentDescription> VKRenderPass::BuildAttachmentDescriptions()
    {
        Vector<vk::AttachmentDescription> result;
        result.reserve(GetAttachmentCount());

        for (const auto& attachmentDesc : m_Desc.Attachments)
        {
            auto& nativeDesc          = result.emplace_back();
            nativeDesc.format         = VKConvert(attachmentDesc.Format);
            nativeDesc.loadOp         = VKConvert(attachmentDesc.LoadOp);
            nativeDesc.storeOp        = VKConvert(attachmentDesc.StoreOp);
            nativeDesc.stencilLoadOp  = VKConvert(attachmentDesc.StencilLoadOp);
            nativeDesc.stencilStoreOp = VKConvert(attachmentDesc.StencilStoreOp);
            nativeDesc.initialLayout  = VKConvert(attachmentDesc.InitialState);
            nativeDesc.finalLayout    = VKConvert(attachmentDesc.FinalState);
        }

        return result;
    }

    Vector<vk::SubpassDescription> VKRenderPass::BuildSubpassDescriptions(
        Vector<VKRenderPass::SubpassAttachmentReferences>& subpassAttachmentReferences) const
    {
        Vector<vk::SubpassDescription> result;
        result.reserve(m_Desc.Subpasses.Size());

        for (size_t i = 0; i < m_Desc.Subpasses.Size(); ++i)
        {
            auto& currentRefs = subpassAttachmentReferences[i];

            auto& nativeDesc             = result.emplace_back();
            nativeDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

            nativeDesc.inputAttachmentCount = static_cast<UInt32>(currentRefs.Input.size());
            nativeDesc.pInputAttachments    = currentRefs.Input.data();

            nativeDesc.colorAttachmentCount = static_cast<UInt32>(currentRefs.RT.size());
            nativeDesc.pColorAttachments    = currentRefs.RT.data();

            nativeDesc.pDepthStencilAttachment =
                currentRefs.DepthStencil.attachment == static_cast<UInt32>(-1) ? nullptr : &currentRefs.DepthStencil;
            nativeDesc.pPreserveAttachments = currentRefs.Preserve;
        }

        return result;
    }

    Vector<vk::AttachmentReference> VKRenderPass::BuildAttachmentReferences(UInt32 subpassIndex, AttachmentType attachmentType)
    {
        Vector<vk::AttachmentReference> result;

        SubpassDesc& currentSubpass = m_Desc.Subpasses[subpassIndex];
        switch (attachmentType)
        {
        case AttachmentType::Input:
            for (auto& inputAttachment : currentSubpass.InputAttachments)
            {
                auto& ref      = result.emplace_back();
                ref.attachment = inputAttachment.Index;
                ref.layout     = VKConvert(inputAttachment.State);
            }
            break;
        case AttachmentType::RenderTarget:
            for (auto& rtAttachment : currentSubpass.RenderTargetAttachments)
            {
                auto& ref      = result.emplace_back();
                ref.attachment = rtAttachment.Index;
                ref.layout     = VKConvert(rtAttachment.State);
            }
            break;
        default:
            FE_UNREACHABLE("Invalid AttachmentType, note that Preserve and DepthStencil attachments are not allowed here");
            return result;
        }

        return result;
    }

    Vector<vk::SubpassDependency> VKRenderPass::BuildSubpassDependencies()
    {
        Vector<vk::SubpassDependency> result;

        static auto validateSubpassIndex = [this](UInt32 index) {
            return index < m_Desc.Subpasses.Size() ? index : VK_SUBPASS_EXTERNAL;
        };

        for (auto& dependency : m_Desc.SubpassDependencies)
        {
            auto& nativeDependency      = result.emplace_back();
            nativeDependency.srcSubpass = validateSubpassIndex(dependency.SourceSubpassIndex);
            nativeDependency.dstSubpass = validateSubpassIndex(dependency.DestinationSubpassIndex);

            nativeDependency.srcStageMask  = VKConvert(dependency.SourcePipelineStage);
            nativeDependency.srcAccessMask = GetAccessMask(dependency.SourceState);

            nativeDependency.dstStageMask  = VKConvert(dependency.DestinationPipelineStage);
            nativeDependency.dstAccessMask = GetAccessMask(dependency.DestinationState);
        }

        return result;
    }
} // namespace FE::GPU
