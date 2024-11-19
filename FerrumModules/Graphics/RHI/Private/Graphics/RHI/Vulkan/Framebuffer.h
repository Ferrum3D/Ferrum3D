#pragma once
#include <Graphics/RHI/Framebuffer.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ImageView;

    struct Framebuffer final : public RHI::Framebuffer
    {
        FE_RTTI_Class(Framebuffer, "E665C1C7-1AD2-48F3-AA54-08090EB5DC76");

        Framebuffer(RHI::Device* device);
        ~Framebuffer() override;

        RHI::ResultCode Init(const RHI::FramebufferDesc& desc) override;

        [[nodiscard]] VkFramebuffer GetNative() const
        {
            return m_nativeFramebuffer;
        }

        [[nodiscard]] const RHI::FramebufferDesc& GetDesc() const override
        {
            return m_desc;
        }

    private:
        VkFramebuffer m_nativeFramebuffer = VK_NULL_HANDLE;
        festd::vector<Rc<ImageView>> m_rTVs;
        RHI::FramebufferDesc m_desc;
    };

    FE_ENABLE_NATIVE_CAST(Framebuffer);
} // namespace FE::Graphics::Vulkan
