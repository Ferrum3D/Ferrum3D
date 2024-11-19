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


    RHI::ResultCode Image::Init(StringSlice name, const RHI::ImageDesc& desc)
    {
        using RHI::ImageBindFlags;
        using RHI::ImageDim;

        m_desc = desc;
        m_name = name;
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
        case ImageDim::kImage1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            break;
        case ImageDim::kImageCubemap:
            FE_AssertMsg(desc.m_arraySize == 6, "Cubemap image must have ArraySize = 6, but got {}", desc.m_arraySize);
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            [[fallthrough]];
        case ImageDim::kImage2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            break;
        case ImageDim::kImage3D:
            imageCI.imageType = VK_IMAGE_TYPE_3D;
            break;
        default:
            FE_AssertMsg(false, "Unknown image dimension");
            break;
        }

        Logger* logger = Env::GetServiceProvider()->ResolveRequired<Logger>();

        imageCI.extent = VKConvert(desc.GetSize());
        if (desc.m_dimension != ImageDim::kImage2D && imageCI.extent.height > 1)
        {
            logger->LogWarning("Expected ImageSize.Height = 1 for a 1D image, but got {}", imageCI.extent.height);
            imageCI.extent.height = 1;
        }
        if (desc.m_dimension != ImageDim::kImage3D && imageCI.extent.depth > 1)
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

        vkCreateImage(NativeCast(m_device), &imageCI, VK_NULL_HANDLE, &m_nativeImage);
        m_owned = true;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeImage);
        nameInfo.pObjectName = m_name.Data();
        vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo);

        vkGetImageMemoryRequirements(NativeCast(m_device), m_nativeImage, &m_memoryRequirements);
        return RHI::ResultCode::kSuccess;
    }


    void Image::AllocateMemory(RHI::MemoryType type)
    {
        RHI::MemoryAllocationDesc desc{};
        desc.m_size = m_memoryRequirements.size;
        desc.m_type = type;

        Rc memory = Rc<DeviceMemory>::DefaultNew(m_device, m_memoryRequirements.memoryTypeBits, desc);
        BindMemory(RHI::DeviceMemorySlice{ memory.Detach() });
        m_memoryOwned = true;
    }


    void Image::BindMemory(const RHI::DeviceMemorySlice& memory)
    {
        m_memory = memory;
        vkBindImageMemory(NativeCast(m_device), m_nativeImage, NativeCast(memory.m_memory), memory.m_byteOffset);
    }


    Image::~Image()
    {
        if (m_owned)
        {
            vkDestroyImage(NativeCast(m_device), m_nativeImage, nullptr);
        }
        if (m_memoryOwned)
        {
            m_memory.m_memory->Release();
        }
    }
} // namespace FE::Graphics::Vulkan
