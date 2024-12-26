#include <FeCore/DI/Activator.h>
#include <Graphics/RHI/Vulkan/Buffer.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceFactory.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    ResourcePool::ResourcePool(RHI::Device* device)
        : m_imagePool("ResourcePool/Images", sizeof(Image), 64 * 1024)
        , m_bufferPool("ResourcePool/Buffers", sizeof(Buffer), 64 * 1024)
    {
        VmaAllocatorCreateInfo createInfo = {};
        createInfo.device = NativeCast(device);
        createInfo.physicalDevice = ImplCast(device)->GetNativeAdapter();
        createInfo.instance = NativeCast(ImplCast(device)->GetDeviceFactory());
        createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        const VkResult result = vmaCreateAllocator(&createInfo, &m_allocator);
        FE_Assert(result == VK_SUCCESS);
    }


    ResourcePool::~ResourcePool()
    {
        vmaDestroyAllocator(m_allocator);
    }


    festd::expected<RHI::Image*, RHI::ResultCode> ResourcePool::CreateImage(Env::Name name, const RHI::ImageDesc& desc)
    {
        return DI::New<Image>(&m_imagePool)
            .map_error([](DI::ResultCode) {
                return RHI::ResultCode::kUnknownError;
            })
            .and_then([this, name, &desc](Image* image) -> festd::expected<RHI::Image*, RHI::ResultCode> {
                const RHI::ResultCode result = image->InitInternal(m_allocator, name, desc);
                if (result == RHI::ResultCode::kSuccess)
                    return image;

                return festd::unexpected(result);
            });
    }


    festd::expected<RHI::Buffer*, RHI::ResultCode> ResourcePool::CreateBuffer(Env::Name name, const RHI::BufferDesc& desc)
    {
        return DI::New<Buffer>(&m_bufferPool)
            .map_error([](DI::ResultCode) {
                return RHI::ResultCode::kUnknownError;
            })
            .and_then([this, name, &desc](Buffer* buffer) -> festd::expected<RHI::Buffer*, RHI::ResultCode> {
                const RHI::ResultCode result = buffer->InitInternal(m_allocator, name, desc);
                if (result == RHI::ResultCode::kSuccess)
                    return buffer;

                return festd::unexpected(result);
            });
    }
} // namespace FE::Graphics::Vulkan
