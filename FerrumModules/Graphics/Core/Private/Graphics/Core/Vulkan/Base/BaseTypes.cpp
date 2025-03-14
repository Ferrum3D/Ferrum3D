#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        //
        // Should be safe to use global pools here since these objects will only
        // be created from within this module.
        //

        constexpr uint32_t kSmallVulkanObjectMaxSize = Math::Max<uint32_t>(sizeof(Semaphore), sizeof(CommandBuffer));

        VulkanObjectPoolType GSmallObjectPool{ "VulkanSmallObjectPool", kSmallVulkanObjectMaxSize };
    } // namespace


    Semaphore::~Semaphore()
    {
        if (m_nativeSemaphore)
        {
            vkDestroySemaphore(NativeCast(m_device), m_nativeSemaphore, nullptr);
            m_nativeSemaphore = VK_NULL_HANDLE;
        }
    }


    Semaphore* Semaphore::Create(Core::Device* device, const Env::Name name)
    {
        FE_PROFILER_ZONE();

        return Rc<Semaphore>::Allocate(&GSmallObjectPool, [=](void* memory) {
            return new (memory) Semaphore(device, name);
        });
    }


    Semaphore::Semaphore(Core::Device* device, const Env::Name name)
    {
        m_device = device;

        const VkDevice vkDevice = NativeCast(device);
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VerifyVulkan(vkCreateSemaphore(vkDevice, &createInfo, nullptr, &m_nativeSemaphore));

        if (name.Valid())
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeSemaphore);
            nameInfo.pObjectName = name.c_str();
            VerifyVulkan(vkSetDebugUtilsObjectNameEXT(vkDevice, &nameInfo));
        }
    }


    CommandBuffer::~CommandBuffer()
    {
        if (m_nativeCommandBuffer)
        {
            vkFreeCommandBuffers(NativeCast(m_device), m_nativeCommandPool, 1, &m_nativeCommandBuffer);
            m_nativeCommandBuffer = VK_NULL_HANDLE;
            m_nativeCommandPool = VK_NULL_HANDLE;
        }
    }


    CommandBuffer* CommandBuffer::Create(Core::Device* device, const Env::Name name, const Core::HardwareQueueKindFlags queueFlags)
    {
        FE_PROFILER_ZONE();

        return Rc<CommandBuffer>::Allocate(&GSmallObjectPool, [=](void* memory) {
            return new (memory) CommandBuffer(device, name, queueFlags);
        });
    }


    CommandBuffer::CommandBuffer(Core::Device* device, const Env::Name name, const Core::HardwareQueueKindFlags queueFlags)
    {
        m_device = device;

        const Device* deviceImpl = ImplCast(device);
        const VkDevice vkDevice = deviceImpl->GetNative();

        m_nativeCommandPool = deviceImpl->GetCommandPool(queueFlags);

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_nativeCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VerifyVulkan(vkAllocateCommandBuffers(vkDevice, &allocInfo, &m_nativeCommandBuffer));

        if (name.Valid())
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeCommandBuffer);
            nameInfo.pObjectName = name.c_str();
            VerifyVulkan(vkSetDebugUtilsObjectNameEXT(vkDevice, &nameInfo));
        }
    }
} // namespace FE::Graphics::Vulkan
