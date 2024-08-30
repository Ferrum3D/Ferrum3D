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
        Register();
    }


    const HAL::ImageDesc& Image::GetDesc()
    {
        return m_Desc;
    }


    HAL::ResultCode Image::Init(StringSlice name, const HAL::ImageDesc& desc)
    {
        using HAL::ImageBindFlags;
        using HAL::ImageDim;

        m_Desc = desc;
        m_Name = name;
        InitState(desc.ArraySize, static_cast<uint16_t>(desc.MipSliceCount));

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        VkImageUsageFlags usage{};
        if ((desc.BindFlags & ImageBindFlags::kDepth) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::kStencil) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::kShaderRead) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::kColor) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::kUnorderedAccess) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::kTransferWrite) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((desc.BindFlags & ImageBindFlags::kTransferRead) != ImageBindFlags::kNone)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        switch (desc.GetDimension())
        {
        case ImageDim::kImage1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            break;
        case ImageDim::kImageCubemap:
            FE_AssertMsg(desc.ArraySize == 6, "Cubemap image must have ArraySize = 6, but got {}", desc.ArraySize);
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
        if (desc.GetDimension() != ImageDim::kImage2D && imageCI.extent.height > 1)
        {
            logger->LogWarning("Expected ImageSize.Height = 1 for a 1D image, but got {}", imageCI.extent.height);
            imageCI.extent.height = 1;
        }
        if (desc.GetDimension() != ImageDim::kImage3D && imageCI.extent.depth > 1)
        {
            logger->LogWarning("Expected ImageSize.Depth = 1 for a non-3D image, but got {}", imageCI.extent.depth);
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

        vkCreateImage(NativeCast(m_pDevice), &imageCI, VK_NULL_HANDLE, &m_NativeImage);
        m_Owned = true;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_NativeImage);
        nameInfo.pObjectName = m_Name.Data();
        vkSetDebugUtilsObjectNameEXT(NativeCast(m_pDevice), &nameInfo);

        vkGetImageMemoryRequirements(NativeCast(m_pDevice), m_NativeImage, &m_MemoryRequirements);
        return HAL::ResultCode::Success;
    }


    void Image::AllocateMemory(HAL::MemoryType type)
    {
        HAL::MemoryAllocationDesc desc{};
        desc.Size = m_MemoryRequirements.size;
        desc.Type = type;

        Rc memory = Rc<DeviceMemory>::DefaultNew(m_pDevice, m_MemoryRequirements.memoryTypeBits, desc);
        BindMemory(HAL::DeviceMemorySlice{ memory.Detach() });
        m_MemoryOwned = true;
    }


    void Image::BindMemory(const HAL::DeviceMemorySlice& memory)
    {
        m_Memory = memory;
        vkBindImageMemory(NativeCast(m_pDevice), m_NativeImage, NativeCast(memory.Memory), memory.ByteOffset);
    }


    Image::~Image()
    {
        if (m_Owned)
        {
            vkDestroyImage(NativeCast(m_pDevice), m_NativeImage, nullptr);
        }
        if (m_MemoryOwned)
        {
            m_Memory.Memory->Release();
        }
    }
} // namespace FE::Graphics::Vulkan
