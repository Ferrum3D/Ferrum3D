#pragma once
#include <HAL/ImageView.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class ImageView : public HAL::ImageView
    {
        HAL::ImageViewDesc m_Desc;
        VkImageView m_NativeView = VK_NULL_HANDLE;

    public:
        explicit ImageView(HAL::Device* pDevice);
        ~ImageView() override;

        HAL::ResultCode Init(const HAL::ImageViewDesc& desc) override;

        [[nodiscard]] const HAL::ImageViewDesc& GetDesc() const override;

        [[nodiscard]] inline VkImageView GetNativeView() const;
    };

    inline VkImageView ImageView::GetNativeView() const
    {
        return m_NativeView;
    }

    FE_ENABLE_IMPL_CAST(ImageView);
} // namespace FE::Graphics::Vulkan
