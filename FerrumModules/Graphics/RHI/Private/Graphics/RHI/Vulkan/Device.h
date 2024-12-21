#pragma once
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Containers/SegmentedVector.h>
#include <festd/vector.h>
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/RHI/Device.h>
#include <Graphics/RHI/DeviceFactory.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct QueueFamilyData
    {
        uint32_t m_familyIndex;
        uint32_t m_queueCount;
        RHI::HardwareQueueKindFlags m_class;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;

        QueueFamilyData(uint32_t idx, uint32_t count, RHI::HardwareQueueKindFlags cmdListClass)
            : m_familyIndex(idx)
            , m_queueCount(count)
            , m_class(cmdListClass)
        {
        }
    };


    struct CommandList;
    struct DeviceFactory;

    struct Device final : public RHI::Device
    {
        FE_RTTI_Class(Device, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        Device(Logger* pLogger, RHI::DeviceFactory* pFactory);
        ~Device() override;

        RHI::ResultCode Init(VkPhysicalDevice nativeAdapter);

        [[nodiscard]] VkDevice GetNative() const
        {
            return m_nativeDevice;
        }

        uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

        VkCommandPool GetCommandPool(RHI::HardwareQueueKindFlags cmdQueueClass)
        {
            for (const auto& queue : m_queueFamilyIndices)
            {
                if (queue.m_class == cmdQueueClass)
                    return queue.m_commandPool;
            }

            FE_AssertMsg(false, "Couldn't find command pool");
            return m_queueFamilyIndices.front().m_commandPool;
        }

        VkCommandPool GetCommandPool(uint32_t queueFamilyIndex)
        {
            for (const auto& queue : m_queueFamilyIndices)
            {
                if (queue.m_familyIndex == queueFamilyIndex)
                    return queue.m_commandPool;
            }

            FE_AssertMsg(false, "Couldn't find command pool");
            return m_queueFamilyIndices.front().m_commandPool;
        }

        uint32_t GetQueueFamilyIndex(RHI::HardwareQueueKindFlags cmdQueueClass)
        {
            for (const auto& queue : m_queueFamilyIndices)
            {
                if (queue.m_class == cmdQueueClass)
                    return queue.m_familyIndex;
            }

            FE_AssertMsg(false, "Couldn't find queue family");
            return static_cast<uint32_t>(-1);
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

        Rc<RHI::CommandQueue> GetCommandQueue(RHI::HardwareQueueKindFlags cmdQueueClass) override;

    private:
        VkDevice m_nativeDevice = VK_NULL_HANDLE;
        VkPhysicalDevice m_nativeAdapter = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_adapterProperties{};

        DeviceFactory* m_deviceFactory = nullptr;
        festd::small_vector<QueueFamilyData> m_queueFamilyIndices;

        void FindQueueFamilies();
    };

    FE_ENABLE_NATIVE_CAST(Device);
} // namespace FE::Graphics::Vulkan
