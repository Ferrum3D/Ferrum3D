#pragma once
#include <FeCore/Memory/Object.h>
#include <FeGPU/ImageView/IImageView.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
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
} // namespace FE::GPU
