#pragma once
#include <FeCore/Logging/Trace.h>
#include <Graphics/Core/Common/Device.h>
#include <Graphics/Core/DeviceFactory.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Sampler.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct QueueFamilyData
    {
        uint32_t m_familyIndex;
        uint32_t m_queueCount;
        Core::HardwareQueueKindFlags m_class;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;

        QueueFamilyData(const uint32_t idx, const uint32_t count, const Core::HardwareQueueKindFlags cmdListClass)
            : m_familyIndex(idx)
            , m_queueCount(count)
            , m_class(cmdListClass)
        {
        }
    };


    template<class TFactory>
    struct VulkanObjectCache final
    {
        using Object = typename TFactory::ObjectType;
        using Desc = typename TFactory::ObjectDesc;

        ~VulkanObjectCache()
        {
            FE_Assert(m_objects.empty(), "Must be explicitly shutdown");
        }

        void Init(Device* device)
        {
            m_device = device;
        }

        void Shutdown()
        {
            for (const auto [hash, object] : m_objects)
            {
                TFactory::Destroy(m_device, object);
            }

            m_objects.clear();
            m_device = nullptr;
        }

        Object Get(const Desc& desc)
        {
            const uint64_t hash = desc.GetHash();
            auto it = m_objects.find(hash);
            if (it != m_objects.end())
                return it->second;

            const Object object = TFactory::Create(m_device, desc);
            m_objects.emplace(hash, object);
            return object;
        }

        // TODO: queue objects for disposal if not accessed for a few frames

        Device* m_device = nullptr;
        festd::unordered_dense_map<uint64_t, Object> m_objects;
    };


    struct CommandList;
    struct DeviceFactory;

    struct Device final : public Common::Device
    {
        FE_RTTI_Class(Device, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        Device(Logger* logger, Core::DeviceFactory* factory);
        ~Device() override;

        void Init(VkPhysicalDevice nativeAdapter);

        [[nodiscard]] VkDevice GetNative() const
        {
            return m_nativeDevice;
        }

        uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

        VkSampler GetSampler(const Core::SamplerState& desc)
        {
            return m_samplerCache.Get(desc);
        }

        VkCommandPool GetCommandPool(const Core::HardwareQueueKindFlags cmdQueueClass) const
        {
            for (const auto& queue : m_queueFamilyIndices)
            {
                if (queue.m_class == cmdQueueClass)
                    return queue.m_commandPool;
            }

            FE_AssertMsg(false, "Couldn't find command pool");
            return m_queueFamilyIndices.front().m_commandPool;
        }

        VkCommandPool GetCommandPool(const uint32_t queueFamilyIndex)
        {
            for (const auto& queue : m_queueFamilyIndices)
            {
                if (queue.m_familyIndex == queueFamilyIndex)
                    return queue.m_commandPool;
            }

            FE_AssertMsg(false, "Couldn't find command pool");
            return m_queueFamilyIndices.front().m_commandPool;
        }

        uint32_t GetQueueFamilyIndex(const Core::HardwareQueueKindFlags cmdQueueClass) const
        {
            for (const auto& queue : m_queueFamilyIndices)
            {
                if (queue.m_class == cmdQueueClass)
                    return queue.m_familyIndex;
            }

            FE_AssertMsg(false, "Couldn't find queue family");
            return kInvalidIndex;
        }

        DeviceFactory* GetDeviceFactory() const
        {
            return m_deviceFactory;
        }

        const VkPhysicalDeviceProperties& GetAdapterProperties() const
        {
            return m_adapterProperties;
        }

        VkPhysicalDevice GetNativeAdapter() const
        {
            return m_nativeAdapter;
        }

        void WaitIdle() override;

    private:
        VkDevice m_nativeDevice = VK_NULL_HANDLE;
        VkPhysicalDevice m_nativeAdapter = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_adapterProperties{};

        VulkanObjectCache<SamplerFactory> m_samplerCache;

        DeviceFactory* m_deviceFactory = nullptr;
        festd::inline_vector<QueueFamilyData> m_queueFamilyIndices;

        void FindQueueFamilies();
    };

    FE_ENABLE_NATIVE_CAST(Device);
} // namespace FE::Graphics::Vulkan
