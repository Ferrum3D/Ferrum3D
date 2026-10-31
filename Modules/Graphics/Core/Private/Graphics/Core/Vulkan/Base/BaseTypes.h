#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/BaseTypes.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    inline constexpr uint32_t kMaxInFlightFrames = 2;

#define FE_DECLARE_VULKAN_OBJECT_POOL(Type)                                                                                      \
    static ::FE::Memory::SpinLockedPool<Type> G##Type##Pool{ FE_MAKE_STRING(Vulkan##Type##ObjectPool) };

    inline VkExtent3D TranslateExtent(const PackedVector3UInt size)
    {
        return VkExtent3D{ size.x, size.y, size.z };
    }


    struct Semaphore final : public Memory::RefCountedObjectBase
    {
        ~Semaphore() override;

        static Semaphore* Create(Core::Device* device, Env::Name name);

        [[nodiscard]] VkSemaphore GetNative() const
        {
            return m_nativeSemaphore;
        }

    private:
        explicit Semaphore(Core::Device* device, Env::Name name);
        void DoRelease() override;

        Core::Device* m_device = nullptr;
        VkSemaphore m_nativeSemaphore = VK_NULL_HANDLE;
    };
} // namespace FE::Graphics::Vulkan
