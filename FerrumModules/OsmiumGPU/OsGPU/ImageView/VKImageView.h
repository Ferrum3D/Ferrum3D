#pragma once
#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKImageView : public Object<IImageView>
    {
        ImageViewDesc m_Desc;
        VKDevice* m_Device;
        vk::UniqueImageView m_NativeView;

    public:
        VKImageView(VKDevice& dev, const ImageViewDesc& desc);

        [[nodiscard]] const ImageViewDesc& GetDesc() const override;

        [[nodiscard]] inline vk::ImageView& GetNativeView();
    };

    inline vk::ImageView& VKImageView::GetNativeView()
    {
        return m_NativeView.get();
    }
} // namespace FE::Osmium
