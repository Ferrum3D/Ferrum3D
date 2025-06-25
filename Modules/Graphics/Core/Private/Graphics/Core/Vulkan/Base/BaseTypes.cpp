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

        constexpr uint32_t kSmallVulkanObjectMaxSize = sizeof(Semaphore);

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

        if (name.IsValid())
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeSemaphore);
            nameInfo.pObjectName = name.c_str();
            VerifyVulkan(vkSetDebugUtilsObjectNameEXT(vkDevice, &nameInfo));
        }
    }
} // namespace FE::Graphics::Vulkan
