#include <OsGPU/Common/VKBaseTypes.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/ImageSubresource.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/Memory/VKDeviceMemory.h>

namespace FE::Osmium
{
    VKImage::VKImage(VKDevice& dev)
        : m_Device(&dev)
    {
    }

    const ImageDesc& VKImage::GetDesc()
    {
        return Desc;
    }

    Shared<IImageView> VKImage::CreateView()
    {
        ImageSubresourceRange range{};
        range.ArraySliceCount = Desc.ArraySize;
        range.MinArraySlice   = 0;
        range.MinMipSlice     = 0;
        range.MipSliceCount   = 1;
        range.AspectFlags     = ImageAspectFlags::RenderTarget;

        ImageViewDesc desc{};
        desc.Format           = Desc.ImageFormat;
        desc.Image            = this;
        desc.Dimension        = Desc.Dimension;
        desc.SubresourceRange = range;
        return m_Device->CreateImageView(desc);
    }

    VKImage::VKImage(VKDevice& dev, const ImageDesc& desc)
        : m_Device(&dev)
        , Desc(desc)
    {
        vk::ImageCreateInfo imageInfo{};
        vk::ImageUsageFlags usage{};
        if ((desc.BindFlags & ImageBindFlags::Depth) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
        }
        if ((desc.BindFlags & ImageBindFlags::Stencil) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst;
        }
        if ((desc.BindFlags & ImageBindFlags::ShaderRead) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eSampled;
        }
        if ((desc.BindFlags & ImageBindFlags::RenderTarget) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
        }
        if ((desc.BindFlags & ImageBindFlags::UnorderedAccess) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eStorage;
        }
        if ((desc.BindFlags & ImageBindFlags::TransferWrite) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eTransferDst;
        }
        if ((desc.BindFlags & ImageBindFlags::TransferRead) != ImageBindFlags::None)
        {
            usage |= vk::ImageUsageFlagBits::eTransferSrc;
        }

        switch (desc.Dimension)
        {
        case ImageDim::Image1D:
            imageInfo.imageType = vk::ImageType::e1D;
            break;
        case ImageDim::ImageCubemap:
            FE_ASSERT_MSG(desc.ArraySize == 6, "Cubemap image must have ArraySize = 6, but got {}", desc.ArraySize);
            imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
            [[fallthrough]];
        case ImageDim::Image2D:
            imageInfo.imageType = vk::ImageType::e2D;
            break;
        case ImageDim::Image3D:
            imageInfo.imageType = vk::ImageType::e3D;
            break;
        default:
            FE_UNREACHABLE("Unknown image dimension");
            break;
        }

        imageInfo.extent = VKConvert(desc.ImageSize);
        if (desc.Dimension != ImageDim::Image2D && imageInfo.extent.height > 1)
        {
            FE_LOG_WARNING("Expected ImageSize.Height = 1 for a 1D image, but got {}", imageInfo.extent.height);
            imageInfo.extent.height = 1;
        }
        if (desc.Dimension != ImageDim::Image3D && imageInfo.extent.depth > 1)
        {
            FE_LOG_WARNING("Expected ImageSize.Depth = 1 for a non-3D image, but got {}", imageInfo.extent.depth);
            imageInfo.extent.depth = 1;
        }
        imageInfo.mipLevels     = desc.MipLevelCount;
        imageInfo.arrayLayers   = desc.ArraySize;
        imageInfo.format        = VKConvert(desc.ImageFormat);
        imageInfo.tiling        = vk::ImageTiling::eOptimal;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage         = usage;
        imageInfo.samples       = static_cast<vk::SampleCountFlagBits>(desc.SampleCount);
        imageInfo.sharingMode   = vk::SharingMode::eExclusive;

        UniqueImage = m_Device->GetNativeDevice().createImageUnique(imageInfo);
        Image       = UniqueImage.get();
    }

    void VKImage::AllocateMemory(MemoryType type)
    {
        auto memoryRequirements = m_Device->GetNativeDevice().getImageMemoryRequirements(Image);
        MemoryAllocationDesc desc{};
        desc.Size = memoryRequirements.size;
        desc.Type = type;
        m_Memory  = MakeShared<VKDeviceMemory>(*m_Device, memoryRequirements.memoryTypeBits, desc);
        BindMemory(static_pointer_cast<IDeviceMemory>(m_Memory), 0);
    }

    void VKImage::BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset)
    {
        m_Memory = static_pointer_cast<VKDeviceMemory>(memory);
        m_Device->GetNativeDevice().bindImageMemory(Image, m_Memory->Memory.get(), offset);
    }
} // namespace FE::Osmium
