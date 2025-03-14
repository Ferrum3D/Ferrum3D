#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/Image.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    ResourcePool::ResourcePool(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        m_device = device;

        VmaAllocatorCreateInfo createInfo = {};
        createInfo.device = NativeCast(device);
        createInfo.physicalDevice = ImplCast(device)->GetNativeAdapter();
        createInfo.instance = NativeCast(ImplCast(device)->GetDeviceFactory());
        createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        VerifyVulkan(vmaCreateAllocator(&createInfo, &m_vmaAllocator));
    }


    ResourcePool::~ResourcePool()
    {
        vmaDestroyAllocator(m_vmaAllocator);
    }


    Core::Image* ResourcePool::CreateImage(const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        Image* image = Image::Create(m_device);
        image->InitInternal(m_vmaAllocator, name, desc);
        return image;
    }


    Core::Buffer* ResourcePool::CreateBuffer(const Env::Name name, const Core::BufferDesc& desc)
    {
        FE_PROFILER_ZONE();

        Buffer* buffer = Buffer::Create(m_device);
        buffer->InitInternal(m_vmaAllocator, name, desc);
        return buffer;
    }
} // namespace FE::Graphics::Vulkan
