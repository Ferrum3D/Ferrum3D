#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/LRUCacheMap.h>
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/EventBus/EventBus.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <HAL/DescriptorDesc.h>
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


    class DescriptorSetLayoutData final
    {
        VkDescriptorSetLayout m_SetLayout;
        uint32_t m_RefCount;

    public:
        inline DescriptorSetLayoutData() = default;

        inline explicit DescriptorSetLayoutData(VkDescriptorSetLayout layout)
            : m_SetLayout(layout)
            , m_RefCount(1)
        {
        }

        [[nodiscard]] inline VkDescriptorSetLayout SetLayout()
        {
            ++m_RefCount;
            return m_SetLayout;
        }

        [[nodiscard]] inline bool Release(VkDevice device)
        {
            if (--m_RefCount == 0)
            {
                vkDestroyDescriptorSetLayout(device, m_SetLayout, VK_NULL_HANDLE);
                return true;
            }

            return false;
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

        Device(Debug::IConsoleLogger* pLogger, HAL::DeviceFactory* pFactory);
        ~Device() override;

        HAL::ResultCode Init(VkPhysicalDevice nativeAdapter);

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

            FE_UNREACHABLE("Couldn't find command pool");
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

            FE_UNREACHABLE("Couldn't find command pool");
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

            FE_UNREACHABLE("Couldn't find queue family");
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

        inline VkDevice GetNativeDevice() const
        {
            return m_NativeDevice;
        }

        void WaitIdle() override;

        VkSemaphore& AddWaitSemaphore();
        VkSemaphore& AddSignalSemaphore();
        uint32_t GetWaitSemaphores(const VkSemaphore** semaphores);
        uint32_t GetSignalSemaphores(const VkSemaphore** semaphores);

        Rc<HAL::CommandQueue> GetCommandQueue(HAL::HardwareQueueKindFlags cmdQueueClass) override;
    };

    FE_ENABLE_IMPL_CAST(Device);
} // namespace FE::Graphics::Vulkan
