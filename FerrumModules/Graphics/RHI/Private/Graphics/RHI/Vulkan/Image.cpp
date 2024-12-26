#include <Graphics/RHI/ImageSubresource.h>
#include <Graphics/RHI/Vulkan/Common/BaseTypes.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceMemory.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageFormat.h>
#include <Graphics/RHI/Vulkan/ImageView.h>
#include <Graphics/RHI/Vulkan/PipelineStates.h>

namespace FE::Graphics::Vulkan
{
    Image::Image(RHI::Device* device)
    {
        m_device = device;
        Register();
    }


    const RHI::ImageDesc& Image::GetDesc()
    {
        return m_desc;
    }


    RHI::ResultCode Image::InitInternal(VmaAllocator allocator, Env::Name name, const RHI::ImageDesc& desc)
    {
        using RHI::ImageBindFlags;
        using RHI::ImageDimension;

        m_desc = desc;
        m_name = name;
        m_vmaAllocator = allocator;

        InitState(desc.m_arraySize, static_cast<uint16_t>(desc.m_mipSliceCount));

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        VkImageUsageFlags usage{};
        if ((desc.m_bindFlags & ImageBindFlags::kDepth) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.m_bindFlags & ImageBindFlags::kStencil) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.m_bindFlags & ImageBindFlags::kShaderRead) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if ((desc.m_bindFlags & ImageBindFlags::kColor) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.m_bindFlags & ImageBindFlags::kUnorderedAccess) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if ((desc.m_bindFlags & ImageBindFlags::kTransferWrite) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.m_bindFlags & ImageBindFlags::kTransferRead) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        switch (desc.m_dimension)
        {
        case ImageDimension::k1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            break;
        case ImageDimension::kCubemap:
            FE_AssertMsg(desc.m_arraySize == 6, "Cubemap image must have ArraySize = 6, but got {}", desc.m_arraySize);
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            [[fallthrough]];
        case ImageDimension::k2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            break;
        case ImageDimension::k3D:
            imageCI.imageType = VK_IMAGE_TYPE_3D;
            break;
        default:
            FE_AssertMsg(false, "Unknown image dimension");
            break;
        }

        Logger* logger = Env::GetServiceProvider()->ResolveRequired<Logger>();

        imageCI.extent = VKConvert(desc.GetSize());
        if (desc.m_dimension != ImageDimension::k2D && imageCI.extent.height > 1)
        {
            logger->LogWarning("Expected ImageSize.Height = 1 for a 1D image, but got {}", imageCI.extent.height);
            imageCI.extent.height = 1;
        }
        if (desc.m_dimension != ImageDimension::k3D && imageCI.extent.depth > 1)
        {
            logger->LogWarning("Expected ImageSize.Depth = 1 for a non-3D image, but got {}", imageCI.extent.depth);
            imageCI.extent.depth = 1;
        }

        imageCI.mipLevels = desc.m_mipSliceCount;
        imageCI.arrayLayers = desc.m_arraySize;
        imageCI.format = VKConvert(desc.m_imageFormat);
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = usage;
        imageCI.samples = GetVKSampleCountFlags(desc.m_sampleCount);
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCI{};
        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;

        switch (desc.m_usage)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case RHI::ResourceUsage::kDeviceOnly:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case RHI::ResourceUsage::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case RHI::ResourceUsage::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        if (vmaCreateImage(allocator, &imageCI, &allocationCI, &m_nativeImage, &m_vmaAllocation, nullptr) != VK_SUCCESS)
            return RHI::ResultCode::kUnknownError;

        vmaSetAllocationName(allocator, m_vmaAllocation, m_name.c_str());

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeImage);
        nameInfo.pObjectName = m_name.c_str();
        if (vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo) != VK_SUCCESS)
            return RHI::ResultCode::kUnknownError;

        return RHI::ResultCode::kSuccess;
    }


    Image::~Image()
    {
        if (m_vmaAllocator != nullptr)
            vmaDestroyImage(m_vmaAllocator, m_nativeImage, m_vmaAllocation);
    }
} // namespace FE::Graphics::Vulkan
