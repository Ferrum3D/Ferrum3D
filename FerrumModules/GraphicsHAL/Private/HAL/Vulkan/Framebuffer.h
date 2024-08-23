#pragma once
#include <HAL/Framebuffer.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class ImageView;

    class Framebuffer final : public HAL::Framebuffer
    {
        VkFramebuffer m_NativeFramebuffer = VK_NULL_HANDLE;
        festd::vector<Rc<ImageView>> m_RTVs;
        HAL::FramebufferDesc m_Desc;

    public:
        FE_RTTI_Class(Framebuffer, "E665C1C7-1AD2-48F3-AA54-08090EB5DC76");

        Framebuffer(HAL::Device* pDevice);
        ~Framebuffer() override;

        HAL::ResultCode Init(const HAL::FramebufferDesc& desc) override;

        [[nodiscard]] inline VkFramebuffer GetNative() const
        {
            return m_NativeFramebuffer;
        }

        [[nodiscard]] inline const HAL::FramebufferDesc& GetDesc() const override;
    };


    inline const HAL::FramebufferDesc& Framebuffer::GetDesc() const
    {
        return m_Desc;
    }

    FE_ENABLE_IMPL_CAST(Framebuffer);
} // namespace FE::Graphics::Vulkan
