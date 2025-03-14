#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Image.h>
#include <Graphics/Core/Vulkan/ImageFormat.h>
#include <Graphics/Core/Vulkan/PipelineStates.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkImageViewType Translate(const Core::ImageDimension dim, const bool isArray)
        {
            switch (dim)
            {
            case Core::ImageDimension::k1D:
                return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            case Core::ImageDimension::k2D:
                return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            case Core::ImageDimension::k3D:
                FE_AssertMsg(!isArray, "Array of 3D images is not allowed");
                return VK_IMAGE_VIEW_TYPE_3D;
            case Core::ImageDimension::kCubemap:
                return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
            default:
                FE_AssertMsg(false, "Invalid ImageDim");
                return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            }
        }


        VkImageAspectFlags GetAspectFlags(const Core::Format format)
        {
            const Core::FormatInfo formatInfo{ format };

            VkImageAspectFlags aspectMask = 0;
            if (Bit::AllSet(formatInfo.m_aspectFlags, Core::ImageAspectFlags::kColor))
                aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
            if (Bit::AllSet(formatInfo.m_aspectFlags, Core::ImageAspectFlags::kDepth))
                aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if (Bit::AllSet(formatInfo.m_aspectFlags, Core::ImageAspectFlags::kStencil))
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

            return aspectMask;
        }

        VulkanObjectPoolType GImagePool{ "VulkanImagePool", sizeof(Image) };
    } // namespace


    Image* Image::Create(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        return Rc<Image>::Allocate(&GImagePool, [device](void* memory) {
            return new (memory) Image(device);
        });
    }


    Image::Image(Core::Device* device)
    {
        m_device = device;
        m_type = Core::ResourceType::kImage;
        Register();
    }


    VkImageView Image::GetSubresourceView(const Core::ImageSubresource& subresource)
    {
        FE_PROFILER_ZONE();

        const uint32_t key = festd::bit_cast<uint32_t>(subresource);
        if (key == festd::bit_cast<uint32_t>(m_wholeImageSubresource))
            return m_view;

        FE_Assert(subresource.m_firstArraySlice + subresource.m_arraySize <= m_desc.m_arraySize);
        FE_Assert(subresource.m_mostDetailedMipSlice + subresource.m_mipSliceCount <= m_desc.m_mipSliceCount);

        const auto it =
            std::lower_bound(m_viewCache.begin(), m_viewCache.end(), key, [](const ViewCacheEntry& lhs, const uint32_t rhs) {
                return lhs.m_key < rhs;
            });

        if (it != m_viewCache.end() && it->m_key == key)
            return it->m_view;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_desc.m_imageFormat);
        viewCI.viewType = Translate(m_desc.m_dimension, m_desc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = GetAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.baseMipLevel = subresource.m_mostDetailedMipSlice;
        viewCI.subresourceRange.levelCount = subresource.m_mipSliceCount;
        viewCI.subresourceRange.baseArrayLayer = subresource.m_firstArraySlice;
        viewCI.subresourceRange.layerCount = subresource.m_arraySize;
        viewCI.image = m_nativeImage;

        VkImageView view = VK_NULL_HANDLE;
        VerifyVulkan(vkCreateImageView(NativeCast(m_device), &viewCI, nullptr, &view));

        m_viewCache.insert(it, ViewCacheEntry{ key, view });
        return view;
    }


    const Core::ImageDesc& Image::GetDesc() const
    {
        return m_desc;
    }


    void Image::InitView()
    {
        FE_PROFILER_ZONE();

        const VkDevice vkDevice = NativeCast(m_device);

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_desc.m_imageFormat);
        viewCI.viewType = Translate(m_desc.m_dimension, m_desc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = GetAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.levelCount = m_desc.m_mipSliceCount;
        viewCI.subresourceRange.layerCount = m_desc.m_arraySize;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.image = m_nativeImage;
        VerifyVulkan(vkCreateImageView(vkDevice, &viewCI, nullptr, &m_view));

        m_wholeImageSubresource.m_firstArraySlice = 0;
        m_wholeImageSubresource.m_mostDetailedMipSlice = 0;
        m_wholeImageSubresource.m_arraySize = m_desc.m_arraySize;
        m_wholeImageSubresource.m_mipSliceCount = m_desc.m_mipSliceCount;
    }


    void Image::InitInternal(const Env::Name name, const Core::ImageDesc& desc, const VkImage nativeImage)
    {
        FE_PROFILER_ZONE();

        m_name = name;
        m_desc = desc;
        m_nativeImage = nativeImage;

        InitView();
    }


    void Image::InitInternal(const VmaAllocator allocator, const Env::Name name, const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        using Core::ImageBindFlags;
        using Core::ImageDimension;

        m_desc = desc;
        m_name = name;
        m_vmaAllocator = allocator;

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        VkImageUsageFlags usage = VK_FLAGS_NONE;
        if (Bit::AllSet(desc.m_bindFlags, ImageBindFlags::kDepthStencilTarget))
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (Bit::AllSet(desc.m_bindFlags, ImageBindFlags::kShaderRead))
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

        if (Bit::AllSet(desc.m_bindFlags, ImageBindFlags::kColorTarget))
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (Bit::AllSet(desc.m_bindFlags, ImageBindFlags::kUnorderedAccess))
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;

        if (Bit::AllSet(desc.m_bindFlags, ImageBindFlags::kTransferDst))
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (Bit::AllSet(desc.m_bindFlags, ImageBindFlags::kTransferSrc))
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

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

        imageCI.extent = VKConvertExtent(desc.GetSize());
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
        imageCI.format = Translate(desc.m_imageFormat);
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

        case Core::ResourceUsage::kDeviceOnly:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case Core::ResourceUsage::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case Core::ResourceUsage::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        // TODO: maybe handle OOM differently
        VerifyVulkan(vmaCreateImage(allocator, &imageCI, &allocationCI, &m_nativeImage, &m_vmaAllocation, nullptr));
        vmaSetAllocationName(allocator, m_vmaAllocation, m_name.c_str());

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeImage);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVulkan(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        InitView();
    }


    Image::~Image()
    {
        const VkDevice vkDevice = NativeCast(m_device);

        for (const ViewCacheEntry& entry : m_viewCache)
            vkDestroyImageView(vkDevice, entry.m_view, nullptr);

        m_viewCache.clear();

        if (m_view != VK_NULL_HANDLE)
            vkDestroyImageView(vkDevice, m_view, nullptr);

        if (m_vmaAllocator != nullptr)
            vmaDestroyImage(m_vmaAllocator, m_nativeImage, m_vmaAllocation);

        m_vmaAllocator = VK_NULL_HANDLE;
        m_vmaAllocation = VK_NULL_HANDLE;
        m_nativeImage = VK_NULL_HANDLE;
    }
} // namespace FE::Graphics::Vulkan
