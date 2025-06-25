#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>

namespace FE::Graphics::Vulkan
{
    FE_DECLARE_VULKAN_OBJECT_POOL(RenderTarget);


    RenderTarget::~RenderTarget()
    {
        Shutdown(NativeCast(m_device));
    }


    RenderTarget* RenderTarget::Create(Core::Device* device)
    {
        return Rc<RenderTarget>::Allocate(&GRenderTargetPool, [device](void* memory) {
            return new (memory) RenderTarget(device);
        });
    }


    void RenderTarget::InitInternal(const VmaAllocator allocator, const Env::Name name, const Core::ImageDesc& desc)
    {
        m_name = name;

        const Core::FormatInfo formatInfo{ desc.m_imageFormat };
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        switch (formatInfo.m_aspectFlags)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ImageAspect::kColor:
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            break;

        case Core::ImageAspect::kDepth:
        case Core::ImageAspect::kStencil:
        case Core::ImageAspect::kDepthStencil:
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            break;
        }

        FE_Assert(!formatInfo.m_isBlockCompressed);

        Image::InitInternal(NativeCast(m_device), name.c_str(), allocator, usage, desc);
    }


    void RenderTarget::InitInternal(const Env::Name name, const Core::ImageDesc& desc, const VkImage image)
    {
        m_name = name;

        Image::InitInternal(NativeCast(m_device), desc, image);
    }


    const Core::ImageDesc& RenderTarget::GetDesc() const
    {
        return m_desc;
    }


    RenderTarget::RenderTarget(Core::Device* device)
    {
        m_device = device;
        m_type = Core::ResourceType::kRenderTarget;
        Register();
    }
} // namespace FE::Graphics::Vulkan
