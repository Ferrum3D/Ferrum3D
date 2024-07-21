#pragma once
#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKImageView : public IImageView
    {
        ImageViewDesc m_Desc;
        VKDevice* m_Device;
        VkImageView m_NativeView;

    public:
        VKImageView(VKDevice& dev, const ImageViewDesc& desc);
        ~VKImageView() override;

        [[nodiscard]] const ImageViewDesc& GetDesc() const override;

        [[nodiscard]] inline VkImageView GetNativeView();
    };

    inline VkImageView VKImageView::GetNativeView()
    {
        return m_NativeView;
    }
} // namespace FE::Osmium
