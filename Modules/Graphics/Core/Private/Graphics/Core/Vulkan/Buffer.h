#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Buffer final : public Core::Buffer
    {
        FE_RTTI_Class(Buffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        ~Buffer() override;

        static Buffer* Create(Core::Device* device);

        [[nodiscard]] VkBuffer GetNative() const
        {
            return m_nativeBuffer;
        }

        void InitInternal(VmaAllocator allocator, Env::Name name, const Core::BufferDesc& desc);

        void* Map() override;
        void Unmap() override;

        [[nodiscard]] const Core::BufferDesc& GetDesc() const override;

    private:
        explicit Buffer(Core::Device* device);

        Core::BufferDesc m_desc;
        VkBuffer m_nativeBuffer = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
    };


    FE_ENABLE_NATIVE_CAST(Buffer);
} // namespace FE::Graphics::Vulkan
