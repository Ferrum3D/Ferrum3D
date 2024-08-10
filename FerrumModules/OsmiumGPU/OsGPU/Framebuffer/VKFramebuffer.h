#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Framebuffer/IFramebuffer.h>

namespace FE::Osmium
{
    class VKDevice;
    class IImageView;

    class VKFramebuffer : public IFramebuffer
    {
        VKDevice* m_Device;
        VkFramebuffer m_NativeFramebuffer;
        eastl::vector<Rc<IImageView>> m_RTVs;
        FramebufferDesc m_Desc;

    public:
        FE_RTTI_Class(VKFramebuffer, "E665C1C7-1AD2-48F3-AA54-08090EB5DC76");

        VKFramebuffer(VKDevice& dev, const FramebufferDesc& desc);
        ~VKFramebuffer() override;

        [[nodiscard]] inline VkFramebuffer GetNativeFramebuffer();

        [[nodiscard]] inline const FramebufferDesc& GetDesc() const override;
    };

    inline VkFramebuffer VKFramebuffer::GetNativeFramebuffer()
    {
        return m_NativeFramebuffer;
    }

    inline const FramebufferDesc& VKFramebuffer::GetDesc() const
    {
        return m_Desc;
    }
} // namespace FE::Osmium
