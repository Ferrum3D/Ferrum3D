#pragma once
#include <FeCore/Math/Vector3Int.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Base/BaseTypes.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    inline constexpr uint32_t kMaxInFlightFrames = 2;

    using VulkanObjectPoolType = Memory::LockedMemoryResource<Memory::PoolAllocator, Threading::SpinLock>;

#define FE_DECLARE_VULKAN_OBJECT_POOL(Type)                                                                                      \
    static ::FE::Graphics::Vulkan::VulkanObjectPoolType G##Type##Pool{ FE_MAKE_STRING(Vulkan##Type##ObjectPool), sizeof(Type) };

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
} // namespace FE::Graphics::Vulkan
