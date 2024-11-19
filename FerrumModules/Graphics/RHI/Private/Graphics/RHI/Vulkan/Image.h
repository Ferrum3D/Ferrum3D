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

        void InitInternal(StringSlice name, const RHI::ImageDesc& desc, VkImage nativeImage)
        {
            m_name = name;
            m_desc = desc;
            m_nativeImage = nativeImage;
        }

        [[nodiscard]] VkImage GetNative() const
        {
            return m_nativeImage;
        }

        [[nodiscard]] const VkMemoryRequirements& GetMemoryRequirements() const
        {
            return m_memoryRequirements;
        }

        RHI::ResultCode Init(StringSlice name, const RHI::ImageDesc& desc) override;

        const RHI::ImageDesc& GetDesc() override;

        void AllocateMemory(RHI::MemoryType type) override;
        void BindMemory(const RHI::DeviceMemorySlice& memory) override;

    private:
        VkMemoryRequirements m_memoryRequirements{};
        VkImage m_nativeImage = VK_NULL_HANDLE;
        RHI::ImageDesc m_desc;

        RHI::DeviceMemorySlice m_memory;
        bool m_memoryOwned = false;
        bool m_owned = false;
    };

    FE_ENABLE_NATIVE_CAST(Image);
} // namespace FE::Graphics::Vulkan
