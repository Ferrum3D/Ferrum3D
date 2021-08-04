#include <FeGPU/Adapter/VKAdapter.h>
#include <FeGPU/Device/VKDevice.h>
#include <algorithm>
#include <FeCore/Console/FeLog.h>
#include <FeGPU/Fence/VKFence.h>

namespace FE::GPU
{
    void VKDevice::FindQueueFamilies()
    {
        auto hasQueueFamily = [this](CommandListClass cmdListClass) {
            return std::any_of(m_QueueFamilyIndices.begin(), m_QueueFamilyIndices.end(), [=](const VKQueueFamilyData& data) {
                return data.Class == cmdListClass;
            });
        };

        auto families = m_NativeAdapter->getQueueFamilyProperties<StdHeapAllocator<vk::QueueFamilyProperties>>();
        for (size_t i = 0; i < families.size(); ++i)
        {
            auto graphics = vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics;
            auto compute  = vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute;
            auto copy     = vk::QueueFlagBits::eTransfer;
            auto idx      = static_cast<uint32_t>(i);

            if ((families[i].queueFlags & graphics) == graphics && !hasQueueFamily(CommandListClass::Graphics))
            {
                m_QueueFamilyIndices.push_back(VKQueueFamilyData(idx, families[i].queueCount, CommandListClass::Graphics));
            }
            else if ((families[i].queueFlags & compute) == compute && !hasQueueFamily(CommandListClass::Compute))
            {
                m_QueueFamilyIndices.push_back(VKQueueFamilyData(idx, families[i].queueCount, CommandListClass::Compute));
            }
            else if ((families[i].queueFlags & copy) == copy && !hasQueueFamily(CommandListClass::Copy))
            {
                m_QueueFamilyIndices.push_back(VKQueueFamilyData(idx, families[i].queueCount, CommandListClass::Copy));
            }
        }
    }

    VKDevice::VKDevice(VKAdapter& adapter)
        : m_Adapter(&adapter)
        , m_NativeAdapter(adapter.GetNativeAdapter())
    {
        FindQueueFamilies();

        auto availableExt = m_NativeAdapter->enumerateDeviceExtensionProperties<StdHeapAllocator<vk::ExtensionProperties>>();
        for (auto& ext : RequiredDeviceExtensions)
        {
            bool found = std::any_of(availableExt.begin(), availableExt.end(), [&](const vk::ExtensionProperties& props) {
                return StringSlice(ext) == props.extensionName.data();
            });
            FE_ASSERT_MSG(found, "Vulkan device extension {} was not found", ext);
        }

        constexpr float queuePriority = 1.0f;
        Vector<vk::DeviceQueueCreateInfo> queuesCI{};
        for (auto& queue : m_QueueFamilyIndices)
        {
            vk::DeviceQueueCreateInfo& queueCI = queuesCI.emplace_back();
            queueCI.queueFamilyIndex = queue.FamilyIndex;
            queueCI.queueCount = 1;
            queueCI.pQueuePriorities = &queuePriority;
        }

        vk::PhysicalDeviceFeatures deviceFeatures{};
        
        vk::DeviceCreateInfo deviceCI = {};
        deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queuesCI.size());
        deviceCI.pQueueCreateInfos = queuesCI.data();
        deviceCI.pEnabledFeatures = &deviceFeatures;
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(RequiredDeviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = RequiredDeviceExtensions.data();

        m_NativeDevice = m_NativeAdapter->createDeviceUnique(deviceCI);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_NativeDevice.get());

        for (auto& queue : m_QueueFamilyIndices)
        {
            vk::CommandPoolCreateInfo poolCI{};
            poolCI.queueFamilyIndex = queue.FamilyIndex;
            queue.CmdPool = m_NativeDevice->createCommandPoolUnique(poolCI);
        }
    }

    vk::Device VKDevice::GetNativeDevice()
    {
        return m_NativeDevice.get();
    }

    RefCountPtr<IFence> VKDevice::CreateFence(uint64_t value)
    {
        return StaticPtrCast<IFence>(MakeShared<VKFence>(*this, value));
    }
} // namespace FE::GPU
