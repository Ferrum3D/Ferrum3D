#pragma once
#include <Graphics/RHI/Buffer.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Buffer final : public RHI::Buffer
    {
        FE_RTTI_Class(Buffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        Buffer(RHI::Device* device);
        ~Buffer() override;

        [[nodiscard]] const VkMemoryRequirements& GetMemoryRequirements() const
        {
            return m_memoryRequirements;
        }

        [[nodiscard]] VkBuffer GetNative() const
        {
            return m_nativeBuffer;
        }

        RHI::ResultCode Init(StringSlice name, const RHI::BufferDesc& desc) override;

        void* Map(uint64_t offset, uint64_t size) override;
        void Unmap() override;

        void AllocateMemory(RHI::MemoryType type) override;
        void BindMemory(const RHI::DeviceMemorySlice& memory) override;

        [[nodiscard]] const RHI::BufferDesc& GetDesc() const override;

    private:
        RHI::BufferDesc m_desc;
        VkMemoryRequirements m_memoryRequirements = {};
        VkBuffer m_nativeBuffer = VK_NULL_HANDLE;

        RHI::DeviceMemorySlice m_memory;
        bool m_memoryOwned = false;

        void DoRelease() override;
    };


    FE_ENABLE_NATIVE_CAST(Buffer);
} // namespace FE::Graphics::Vulkan
