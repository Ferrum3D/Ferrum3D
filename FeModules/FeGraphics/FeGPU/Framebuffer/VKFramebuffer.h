#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Framebuffer/IFramebuffer.h>

namespace FE::GPU
{
    class VKDevice;
    class IImageView;

    class VKFramebuffer : public Object<IFramebuffer>
    {
        VKDevice* m_Device;
        vk::UniqueFramebuffer m_NativeFramebuffer;
        Vector<RefCountPtr<IImageView>> m_RTVs;
        FramebufferDesc m_Desc;

    public:
        FE_CLASS_RTTI(VKFramebuffer, "E665C1C7-1AD2-48F3-AA54-08090EB5DC76");

        VKFramebuffer(VKDevice& dev, const FramebufferDesc& desc);

        [[nodiscard]] inline vk::Framebuffer& GetNativeFramebuffer();

        [[nodiscard]] inline const FramebufferDesc& GetDesc() const override;
    };

    inline vk::Framebuffer& VKFramebuffer::GetNativeFramebuffer()
    {
        return m_NativeFramebuffer.get();
    }

    inline const FramebufferDesc& VKFramebuffer::GetDesc() const
    {
        return m_Desc;
    }
} // namespace FE::GPU
