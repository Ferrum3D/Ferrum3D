#pragma once
#include <HAL/Buffer.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Buffer final : public HAL::Buffer
    {
        HAL::BufferDesc m_Desc;
        VkMemoryRequirements m_MemoryRequirements = {};
        VkBuffer m_NativeBuffer = VK_NULL_HANDLE;

        HAL::DeviceMemorySlice m_Memory;
        bool m_MemoryOwned = false;

        void DoRelease() override;

    public:
        FE_RTTI_Class(Buffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        Buffer(HAL::Device* pDevice);
        ~Buffer() override;

        [[nodiscard]] inline const VkMemoryRequirements& GetMemoryRequirements() const
        {
            return m_MemoryRequirements;
        }

        [[nodiscard]] inline VkBuffer GetNative() const
        {
            return m_NativeBuffer;
        }

        HAL::ResultCode Init(StringSlice name, const HAL::BufferDesc& desc) override;

        void* Map(uint64_t offset, uint64_t size) override;
        void Unmap() override;

        void AllocateMemory(HAL::MemoryType type) override;
        void BindMemory(const HAL::DeviceMemorySlice& memory) override;

        [[nodiscard]] const HAL::BufferDesc& GetDesc() const override;
    };


    FE_ENABLE_IMPL_CAST(Buffer);
} // namespace FE::Graphics::Vulkan
