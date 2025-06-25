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
    } // namespace


    VkImageView Image::GetSubresourceView(const VkDevice device, const Core::ImageSubresource& subresource) const
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
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.baseMipLevel = subresource.m_mostDetailedMipSlice;
        viewCI.subresourceRange.levelCount = subresource.m_mipSliceCount;
        viewCI.subresourceRange.baseArrayLayer = subresource.m_firstArraySlice;
        viewCI.subresourceRange.layerCount = subresource.m_arraySize;
        viewCI.image = m_nativeImage;

        VkImageView view = VK_NULL_HANDLE;
        VerifyVulkan(vkCreateImageView(device, &viewCI, nullptr, &view));

        m_viewCache.insert(it, ViewCacheEntry{ key, view });
        return view;
    }


    void Image::InitView(const VkDevice device)
    {
        FE_PROFILER_ZONE();

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_desc.m_imageFormat);
        viewCI.viewType = Translate(m_desc.m_dimension, m_desc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.levelCount = m_desc.m_mipSliceCount;
        viewCI.subresourceRange.layerCount = m_desc.m_arraySize;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.image = m_nativeImage;
        VerifyVulkan(vkCreateImageView(device, &viewCI, nullptr, &m_view));

        m_wholeImageSubresource.m_firstArraySlice = 0;
        m_wholeImageSubresource.m_mostDetailedMipSlice = 0;
        m_wholeImageSubresource.m_arraySize = m_desc.m_arraySize;
        m_wholeImageSubresource.m_mipSliceCount = m_desc.m_mipSliceCount;
    }


    void Image::InitInternal(const VkDevice device, const Core::ImageDesc& desc, const VkImage nativeImage)
    {
        FE_PROFILER_ZONE();

        m_desc = desc;
        m_nativeImage = nativeImage;

        InitView(device);
    }


    void Image::InitInternal(const VkDevice device, const char* name, const VmaAllocator allocator, const VkImageUsageFlags usage,
                             const Core::ImageDesc& desc)
    {
        FE_PROFILER_ZONE();

        using Core::ImageDimension;

        m_desc = desc;
        m_vmaAllocator = allocator;

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.usage = usage;

        switch (desc.m_dimension)
        {
        case ImageDimension::k1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            FE_Assert(desc.m_height == 1);
            FE_Assert(desc.m_depth == 1);
            break;
        case ImageDimension::kCubemap:
            FE_AssertMsg(desc.m_arraySize == 6, "Cubemap image must have exactly 6 slices, but got {}", desc.m_arraySize);
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            [[fallthrough]];
        case ImageDimension::k2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            FE_Assert(desc.m_depth == 1);
            break;
        case ImageDimension::k3D:
            imageCI.imageType = VK_IMAGE_TYPE_3D;
            break;
        default:
            FE_AssertMsg(false, "Unknown image dimension");
            break;
        }

        imageCI.extent = VKConvertExtent(desc.GetSize());
        imageCI.mipLevels = desc.m_mipSliceCount;
        imageCI.arrayLayers = desc.m_arraySize;
        imageCI.format = Translate(desc.m_imageFormat);
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.samples = GetVKSampleCountFlags(desc.m_sampleCount);
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCI{};
        allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        // TODO: maybe handle OOM differently
        VerifyVulkan(vmaCreateImage(allocator, &imageCI, &allocationCI, &m_nativeImage, &m_vmaAllocation, nullptr));
        vmaSetAllocationName(allocator, m_vmaAllocation, name);

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeImage);
        nameInfo.pObjectName = name;
        VerifyVulkan(vkSetDebugUtilsObjectNameEXT(device, &nameInfo));

        InitView(device);
    }


    void Image::Shutdown(const VkDevice device)
    {
        for (const ViewCacheEntry& entry : m_viewCache)
            vkDestroyImageView(device, entry.m_view, nullptr);

        m_viewCache.clear();

        if (m_view != VK_NULL_HANDLE)
            vkDestroyImageView(device, m_view, nullptr);

        if (m_vmaAllocator != nullptr)
            vmaDestroyImage(m_vmaAllocator, m_nativeImage, m_vmaAllocation);

        m_vmaAllocator = VK_NULL_HANDLE;
        m_vmaAllocation = VK_NULL_HANDLE;
        m_nativeImage = VK_NULL_HANDLE;
    }
} // namespace FE::Graphics::Vulkan
