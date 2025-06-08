#pragma once
#include <FeCore/Math/Vector3Int.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Base/BaseTypes.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    using VulkanObjectPoolType = Memory::LockedMemoryResource<Memory::PoolAllocator, Threading::SpinLock>;

    inline VkExtent3D VKConvertExtent(const PackedVector3UInt size)
    {
        return VkExtent3D{ size.x, size.y, size.z };
    }


    inline VkOffset3D VKConvertOffset(const PackedVector3Int offset)
    {
        return VkOffset3D{ offset.x, offset.y, offset.z };
    }


    struct Semaphore final : public Core::DeviceObject
    {
        ~Semaphore() override;

        static Semaphore* Create(Core::Device* device, Env::Name name);

        [[nodiscard]] VkSemaphore GetNative() const
        {
            return m_nativeSemaphore;
        }

    private:
        explicit Semaphore(Core::Device* device, Env::Name name);

        VkSemaphore m_nativeSemaphore = VK_NULL_HANDLE;
    };


    struct CommandBuffer final : public Core::DeviceObject
    {
        ~CommandBuffer() override;

        static CommandBuffer* Create(Core::Device* device, Env::Name name, Core::HardwareQueueKindFlags queueFlags);

        [[nodiscard]] VkCommandBuffer GetNative() const
        {
            return m_nativeCommandBuffer;
        }

    private:
        explicit CommandBuffer(Core::Device* device, Env::Name name, Core::HardwareQueueKindFlags queueFlags);

        VkCommandBuffer m_nativeCommandBuffer = VK_NULL_HANDLE;
        VkCommandPool m_nativeCommandPool = VK_NULL_HANDLE;
    };
} // namespace FE::Graphics::Vulkan
