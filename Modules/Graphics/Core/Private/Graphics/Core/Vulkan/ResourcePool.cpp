#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>
#include <Graphics/Core/Vulkan/Texture.h>

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


    Core::Texture* ResourcePool::CreateTexture(const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        Texture* texture = Texture::Create(m_device);
        texture->InitInternal(m_vmaAllocator, name, desc);
        return texture;
    }


    Core::RenderTarget* ResourcePool::CreateRenderTarget(const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        RenderTarget* renderTarget = RenderTarget::Create(m_device);
        renderTarget->InitInternal(m_vmaAllocator, name, desc);
        return renderTarget;
    }


    Core::Buffer* ResourcePool::CreateBuffer(const Env::Name name, const Core::BufferDesc& desc)
    {
        FE_PROFILER_ZONE();

        Buffer* buffer = Buffer::Create(m_device);
        buffer->InitInternal(m_vmaAllocator, name, desc);
        return buffer;
    }
} // namespace FE::Graphics::Vulkan
