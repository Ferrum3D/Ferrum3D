#pragma once
#include <HAL/Fence.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Fence final : public HAL::Fence
    {
        VkFence m_NativeFence = VK_NULL_HANDLE;

    public:
        FE_RTTI_Class(Fence, "78363647-3381-46F2-97B1-2A1AC8AFC3C1");

        Fence(HAL::Device* pDevice);
        ~Fence() override;

        HAL::ResultCode Init(HAL::FenceState initialState) override;

        void SignalOnCPU() override;
        void WaitOnCPU() override;
        void Reset() override;
        HAL::FenceState GetState() override;

        [[nodiscard]] inline VkFence GetNative() const
        {
            return m_NativeFence;
        }
    };

    FE_ENABLE_IMPL_CAST(Fence);
} // namespace FE::Graphics::Vulkan
