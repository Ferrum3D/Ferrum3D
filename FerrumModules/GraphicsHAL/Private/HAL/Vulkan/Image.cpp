#include <HAL/ImageSubresource.h>
#include <HAL/Vulkan/Common/BaseTypes.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceMemory.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/ImageFormat.h>
#include <HAL/Vulkan/ImageView.h>
#include <HAL/Vulkan/PipelineStates.h>

namespace FE::Graphics::Vulkan
{
    Image::Image(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    const HAL::ImageDesc& Image::GetDesc()
    {
        return Desc;
    }


    HAL::ResultCode Image::Init(const HAL::ImageDesc& desc)
    {
        using HAL::ImageBindFlags;
        using HAL::ImageDim;

        Desc = desc;
        InitState(desc.ArraySize, static_cast<uint16_t>(desc.MipSliceCount));

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        VkImageUsageFlags usage{};
        if ((desc.BindFlags & ImageBindFlags::Depth) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::Stencil) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::ShaderRead) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::Color) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::UnorderedAccess) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::TransferWrite) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::TransferRead) != ImageBindFlags::None)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        switch (desc.Dimension)
        {
        case ImageDim::Image1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            break;
        case ImageDim::ImageCubemap:
            FE_ASSERT_MSG(desc.ArraySize == 6, "Cubemap image must have ArraySize = 6, but got {}", desc.ArraySize);
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            [[fallthrough]];
        case ImageDim::Image2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            break;
        case ImageDim::Image3D:
            imageCI.imageType = VK_IMAGE_TYPE_3D;
            break;
        default:
            FE_UNREACHABLE("Unknown image dimension");
            break;
        }

        imageCI.extent = VKConvert(desc.ImageSize);
        if (desc.Dimension != ImageDim::Image2D && imageCI.extent.height > 1)
        {
            FE_LOG_WARNING("Expected ImageSize.Height = 1 for a 1D image, but got {}", imageCI.extent.height);
            imageCI.extent.height = 1;
        }
        if (desc.Dimension != ImageDim::Image3D && imageCI.extent.depth > 1)
        {
            FE_LOG_WARNING("Expected ImageSize.Depth = 1 for a non-3D image, but got {}", imageCI.extent.depth);
            imageCI.extent.depth = 1;
        }
        imageCI.mipLevels = desc.MipSliceCount;
        imageCI.arrayLayers = desc.ArraySize;
        imageCI.format = VKConvert(desc.ImageFormat);
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = usage;
        imageCI.samples = GetVKSampleCountFlags(desc.SampleCount);
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(ImplCast(m_pDevice)->GetNativeDevice(), &imageCI, VK_NULL_HANDLE, &NativeImage);
        m_Owned = true;

        vkGetImageMemoryRequirements(ImplCast(m_pDevice)->GetNativeDevice(), NativeImage, &MemoryRequirements);
        return HAL::ResultCode::Success;
    }


    void Image::AllocateMemory(HAL::MemoryType type)
    {
        HAL::MemoryAllocationDesc desc{};
        desc.Size = MemoryRequirements.size;
        desc.Type = type;
        Rc memory = Rc<DeviceMemory>::DefaultNew(m_pDevice, MemoryRequirements.memoryTypeBits, desc);
        BindMemory(HAL::DeviceMemorySlice{ memory.Detach() });
        m_MemoryOwned = true;
    }


    void Image::BindMemory(const HAL::DeviceMemorySlice& memory)
    {
        m_Memory = memory;
        vkBindImageMemory(
            ImplCast(m_pDevice)->GetNativeDevice(), NativeImage, ImplCast(memory.Memory)->Memory, memory.ByteOffset);
    }


    Image::~Image()
    {
        if (m_Owned)
        {
            vkDestroyImage(ImplCast(m_pDevice)->GetNativeDevice(), NativeImage, nullptr);
        }
        if (m_MemoryOwned)
        {
            m_Memory.Memory->Release();
        }
    }
} // namespace FE::Graphics::Vulkan
