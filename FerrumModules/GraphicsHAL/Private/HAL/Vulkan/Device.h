#pragma once
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Containers/SmallVector.h>
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/Logging/Trace.h>
#include <HAL/Device.h>
#include <HAL/DeviceFactory.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct QueueFamilyData
    {
        uint32_t FamilyIndex;
        uint32_t QueueCount;
        HAL::HardwareQueueKindFlags Class;
        VkCommandPool CmdPool = VK_NULL_HANDLE;

        QueueFamilyData(uint32_t idx, uint32_t count, HAL::HardwareQueueKindFlags cmdListClass)
            : FamilyIndex(idx)
            , QueueCount(count)
            , Class(cmdListClass)
        {
        }
    };


    class CommandList;
    class DeviceFactory;

    class Device final : public HAL::Device
    {
        VkDevice m_NativeDevice = VK_NULL_HANDLE;
        VkPhysicalDevice m_NativeAdapter = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_AdapterProperties{};

        DeviceFactory* m_pDeviceFactory = nullptr;
        festd::small_vector<QueueFamilyData> m_QueueFamilyIndices;

        void FindQueueFamilies();

    public:
        FE_RTTI_Class(Device, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        Device(Logger* pLogger, HAL::DeviceFactory* pFactory);
        ~Device() override;

        HAL::ResultCode Init(VkPhysicalDevice nativeAdapter);

        [[nodiscard]] VkDevice GetNative() const
        {
            return m_NativeDevice;
        }

        uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

        VkCommandPool GetCommandPool(HAL::HardwareQueueKindFlags cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.Class == cmdQueueClass)
                {
                    return queue.CmdPool;
                }
            }

            FE_AssertMsg(false, "Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool;
        }

        VkCommandPool GetCommandPool(uint32_t queueFamilyIndex)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.FamilyIndex == queueFamilyIndex)
                {
                    return queue.CmdPool;
                }
            }

            FE_AssertMsg(false, "Couldn't find command pool");
            return m_QueueFamilyIndices.front().CmdPool;
        }

        uint32_t GetQueueFamilyIndex(HAL::HardwareQueueKindFlags cmdQueueClass)
        {
            for (auto& queue : m_QueueFamilyIndices)
            {
                if (queue.Class == cmdQueueClass)
                {
                    return queue.FamilyIndex;
                }
            }

            FE_AssertMsg(false, "Couldn't find queue family");
            return static_cast<uint32_t>(-1);
        }

        DeviceFactory* GetDeviceFactory() const
        {
            return m_pDeviceFactory;
        }

        const VkPhysicalDeviceProperties& GetAdapterProperties() const
        {
            return m_AdapterProperties;
        }

        VkPhysicalDevice GetNativeAdapter() const
        {
            return m_NativeAdapter;
        }

        void WaitIdle() override;

        Rc<HAL::CommandQueue> GetCommandQueue(HAL::HardwareQueueKindFlags cmdQueueClass) override;
    };

    FE_ENABLE_NATIVE_CAST(Device);
} // namespace FE::Graphics::Vulkan
