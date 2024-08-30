#pragma once
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Containers/SegmentedVector.h>
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

        inline QueueFamilyData(uint32_t idx, uint32_t count, HAL::HardwareQueueKindFlags cmdListClass)
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
        eastl::vector<QueueFamilyData> m_QueueFamilyIndices;

        eastl::vector<VkSemaphore> m_WaitSemaphores;
        eastl::vector<VkSemaphore> m_SignalSemaphores;

        void FindQueueFamilies();

    public:
        FE_RTTI_Class(Device, "7AE4B802-75AF-439E-AA48-BC72761B7B72");

        Device(Logger* pLogger, HAL::DeviceFactory* pFactory);
        ~Device() override;

        HAL::ResultCode Init(VkPhysicalDevice nativeAdapter);

        [[nodiscard]] inline VkDevice GetNative() const
        {
            return m_NativeDevice;
        }

        uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

        inline VkCommandPool GetCommandPool(HAL::HardwareQueueKindFlags cmdQueueClass)
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

        inline VkCommandPool GetCommandPool(uint32_t queueFamilyIndex)
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

        inline uint32_t GetQueueFamilyIndex(HAL::HardwareQueueKindFlags cmdQueueClass)
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

        inline DeviceFactory* GetDeviceFactory() const
        {
            return m_pDeviceFactory;
        }

        inline const VkPhysicalDeviceProperties& GetAdapterProperties() const
        {
            return m_AdapterProperties;
        }

        inline VkPhysicalDevice GetNativeAdapter() const
        {
            return m_NativeAdapter;
        }

        void WaitIdle() override;

        VkSemaphore& AddWaitSemaphore();
        VkSemaphore& AddSignalSemaphore();
        uint32_t GetWaitSemaphores(const VkSemaphore** semaphores);
        uint32_t GetSignalSemaphores(const VkSemaphore** semaphores);

        Rc<HAL::CommandQueue> GetCommandQueue(HAL::HardwareQueueKindFlags cmdQueueClass) override;
    };

    FE_ENABLE_NATIVE_CAST(Device);
} // namespace FE::Graphics::Vulkan
