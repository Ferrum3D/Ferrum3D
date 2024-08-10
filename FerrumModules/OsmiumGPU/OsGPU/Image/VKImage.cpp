#include <OsGPU/Common/VKBaseTypes.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/ImageSubresource.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/Memory/VKDeviceMemory.h>
#include <OsGPU/Pipeline/VKPipelineStates.h>

namespace FE::Osmium
{
    VKImage::VKImage(VKDevice& dev)
        : ImageBase(1, 1)
        , m_Device(&dev)
    {
    }

    const ImageDesc& VKImage::GetDesc()
    {
        return Desc;
    }

    Rc<IImageView> VKImage::CreateView(ImageAspectFlags aspectFlags)
    {
        ImageSubresourceRange range{};
        range.ArraySliceCount = Desc.ArraySize;
        range.MinArraySlice = 0;
        range.MinMipSlice = 0;
        range.MipSliceCount = static_cast<uint16_t>(Desc.MipSliceCount);
        range.AspectFlags = aspectFlags;

        ImageViewDesc desc{};
        desc.Format = Desc.ImageFormat;
        desc.Image = this;
        desc.Dimension = Desc.Dimension;
        desc.SubresourceRange = range;
        return m_Device->CreateImageView(desc);
    }

    VKImage::VKImage(VKDevice& dev, const ImageDesc& desc)
        : ImageBase(desc.ArraySize, static_cast<uint16_t>(desc.MipSliceCount))
        , m_Device(&dev)
        , Desc(desc)
    {
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

        vkCreateImage(m_Device->GetNativeDevice(), &imageCI, VK_NULL_HANDLE, &Image);
        m_Owned = true;

        vkGetImageMemoryRequirements(m_Device->GetNativeDevice(), Image, &MemoryRequirements);
    }

    void VKImage::AllocateMemory(MemoryType type)
    {
        MemoryAllocationDesc desc{};
        desc.Size = MemoryRequirements.size;
        desc.Type = type;
        Rc memory = Rc<VKDeviceMemory>::DefaultNew(*m_Device, MemoryRequirements.memoryTypeBits, desc);
        BindMemory(DeviceMemorySlice(memory.Detach()));
        m_MemoryOwned = true;
    }

    void VKImage::BindMemory(const DeviceMemorySlice& memory)
    {
        m_Memory = memory;
        auto vkMemory = fe_assert_cast<VKDeviceMemory*>(memory.Memory)->Memory;
        vkBindImageMemory(m_Device->GetNativeDevice(), Image, vkMemory, memory.ByteOffset);
    }

    FE_VK_OBJECT_DELETER(Image);

    VKImage::~VKImage()
    {
        if (m_Owned)
        {
            FE_DELETE_VK_OBJECT(Image, Image);
        }
        if (m_MemoryOwned)
        {
            m_Memory.Memory->Release();
        }
    }
} // namespace FE::Osmium
