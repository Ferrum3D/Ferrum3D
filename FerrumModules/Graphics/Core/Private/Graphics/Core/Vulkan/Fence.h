#pragma once
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Fence final : public Core::Fence
    {
        FE_RTTI_Class(Fence, "78363647-3381-46F2-97B1-2A1AC8AFC3C1");

        ~Fence() override;

        static Fence* Create(Core::Device* device);

        Core::ResultCode Init(uint64_t initialValue) override;

        void Signal(uint64_t value) override;
        void Wait(uint64_t value) override;
        uint64_t GetCompletedValue() override;

        [[nodiscard]] VkSemaphore GetNative() const
        {
            return m_timelineSemaphore;
        }

    private:
        explicit Fence(Core::Device* device);

        VkSemaphore m_timelineSemaphore = VK_NULL_HANDLE;
    };

    FE_ENABLE_NATIVE_CAST(Fence);
} // namespace FE::Graphics::Vulkan
