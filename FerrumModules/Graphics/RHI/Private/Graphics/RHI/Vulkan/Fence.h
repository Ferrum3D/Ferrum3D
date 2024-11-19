#pragma once
#include <Graphics/RHI/Fence.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Fence final : public RHI::Fence
    {
        FE_RTTI_Class(Fence, "78363647-3381-46F2-97B1-2A1AC8AFC3C1");

        Fence(RHI::Device* device);
        ~Fence() override;

        RHI::ResultCode Init(uint64_t initialValue) override;

        void Signal(uint64_t value) override;
        void Wait(uint64_t value) override;
        uint64_t GetCompletedValue() override;

        [[nodiscard]] VkSemaphore GetNative() const
        {
            return m_timelineSemaphore;
        }

    private:
        VkSemaphore m_timelineSemaphore = VK_NULL_HANDLE;
    };

    FE_ENABLE_NATIVE_CAST(Fence);
} // namespace FE::Graphics::Vulkan
