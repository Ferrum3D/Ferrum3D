#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    FE_DECLARE_VULKAN_OBJECT_POOL(Semaphore);


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
        return new (GSemaphorePool.AllocateMemory()) Semaphore(device, name);
    }


    Semaphore::Semaphore(Core::Device* device, const Env::Name name)
    {
        m_device = device;

        const VkDevice vkDevice = NativeCast(device);
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VerifyVk(vkCreateSemaphore(vkDevice, &createInfo, nullptr, &m_nativeSemaphore));

        if (name.IsValid())
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeSemaphore);
            nameInfo.pObjectName = name.c_str();
            VerifyVk(vkSetDebugUtilsObjectNameEXT(vkDevice, &nameInfo));
        }
    }


    void Semaphore::DestroyObject()
    {
        GSemaphorePool.Delete(this);
    }
} // namespace FE::Graphics::Vulkan
