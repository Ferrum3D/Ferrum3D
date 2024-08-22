#pragma once
#include <HAL/ImageBase.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Image final : public HAL::ImageBase
    {
        HAL::DeviceMemorySlice m_Memory;
        bool m_MemoryOwned = false;
        bool m_Owned = false;

    public:
        VkMemoryRequirements MemoryRequirements{};
        VkImage NativeImage = VK_NULL_HANDLE;
        HAL::ImageDesc Desc;

        FE_RTTI_Class(Image, "9726C432-92C1-489C-9623-55330B3530E8");

        explicit Image(HAL::Device* pDevice);
        ~Image() override;

        HAL::ResultCode Init(const HAL::ImageDesc& desc) override;

        const HAL::ImageDesc& GetDesc() override;

        void AllocateMemory(HAL::MemoryType type) override;
        void BindMemory(const HAL::DeviceMemorySlice& memory) override;
    };

    FE_ENABLE_IMPL_CAST(Image);
} // namespace FE::Graphics::Vulkan
