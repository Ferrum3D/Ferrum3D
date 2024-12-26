#pragma once
#include <Graphics/RHI/ImageBase.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Image final : public RHI::ImageBase
    {
        FE_RTTI_Class(Image, "9726C432-92C1-489C-9623-55330B3530E8");

        explicit Image(RHI::Device* device);
        ~Image() override;

        void InitInternal(Env::Name name, const RHI::ImageDesc& desc, VkImage nativeImage)
        {
            m_name = name;
            m_desc = desc;
            m_nativeImage = nativeImage;
        }

        RHI::ResultCode InitInternal(VmaAllocator allocator, Env::Name name, const RHI::ImageDesc& desc);

        [[nodiscard]] VkImage GetNative() const
        {
            return m_nativeImage;
        }

        const RHI::ImageDesc& GetDesc() override;

    private:
        VkImage m_nativeImage = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
        RHI::ImageDesc m_desc;
    };

    FE_ENABLE_NATIVE_CAST(Image);
} // namespace FE::Graphics::Vulkan
