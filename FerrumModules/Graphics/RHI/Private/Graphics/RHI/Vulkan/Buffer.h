#pragma once
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Buffer final : public RHI::Buffer
    {
        FE_RTTI_Class(Buffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        explicit Buffer(RHI::Device* device);
        ~Buffer() override;

        [[nodiscard]] VkBuffer GetNative() const
        {
            return m_nativeBuffer;
        }

        RHI::ResultCode InitInternal(VmaAllocator allocator, Env::Name name, const RHI::BufferDesc& desc);

        void* Map(uint32_t offset, uint32_t size) override;
        void Unmap() override;

        [[nodiscard]] const RHI::BufferDesc& GetDesc() const override;

    private:
        RHI::BufferDesc m_desc;
        VkBuffer m_nativeBuffer = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
    };


    FE_ENABLE_NATIVE_CAST(Buffer);
} // namespace FE::Graphics::Vulkan
