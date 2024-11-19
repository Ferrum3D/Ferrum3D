#pragma once
#include <Graphics/RHI/ImageView.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ImageView final : public RHI::ImageView
    {
        FE_RTTI_Class(ImageView, "8FB20182-96B7-4D3D-8148-9FE2142A0875");

        explicit ImageView(RHI::Device* device);
        ~ImageView() override;

        using NativeType = VkImageView;

        RHI::ResultCode Init(const RHI::ImageViewDesc& desc) override;

        [[nodiscard]] const RHI::ImageViewDesc& GetDesc() const override;

        [[nodiscard]] VkImageView GetNative() const
        {
            return m_nativeView;
        }

    private:
        RHI::ImageViewDesc m_desc;
        VkImageView m_nativeView = VK_NULL_HANDLE;
    };


    FE_ENABLE_NATIVE_CAST(ImageView);
} // namespace FE::Graphics::Vulkan
