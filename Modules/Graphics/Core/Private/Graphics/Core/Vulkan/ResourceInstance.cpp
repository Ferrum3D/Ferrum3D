#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Format.h>
#include <Graphics/Core/Vulkan/PipelineStates.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        Memory::SpinLockedPoolAllocator GBufferInstancePool{ "Graphics/BufferInstancePool", sizeof(BufferInstance) };
        Memory::SpinLockedPoolAllocator GTextureInstancePool{ "Graphics/TextureInstancePool", sizeof(TextureInstance) };


        VkBufferUsageFlags TranslateBufferUsage(const Core::BarrierAccessFlags accessFlags, const bool isTexelBuffer)
        {
            FE_Assert((accessFlags & Core::BarrierAccessFlags::kAllBufferAccessMask) == accessFlags);

            VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            if (Bit::AnySet(accessFlags, Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite))
            {
                usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
                usage |= isTexelBuffer ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kIndexBuffer))
                usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kVertexBuffer))
                usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kConstantBuffer))
                usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kIndirectArgument))
                usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kAccelerationStructureRead))
                usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kAccelerationStructureWrite))
                usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

            return usage;
        }


        VkImageViewType Translate(const Core::TextureDimension dim, const bool isArray)
        {
            switch (dim)
            {
            case Core::TextureDimension::k1D:
                return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            case Core::TextureDimension::k2D:
                return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            case Core::TextureDimension::k3D:
                FE_AssertMsg(!isArray, "Array of 3D images is not allowed");
                return VK_IMAGE_VIEW_TYPE_3D;
            case Core::TextureDimension::kCubemap:
                return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
            default:
                FE_AssertMsg(false, "Invalid ImageDim");
                return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            }
        }


        VkImageUsageFlags GetImageUsage(const Core::BarrierAccessFlags accessFlags)
        {
            FE_Assert((accessFlags & Core::BarrierAccessFlags::kAllTextureAccessMask) == accessFlags);

            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kRenderTarget))
                usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kDepthStencilWrite))
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kShadingRateSource))
                usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kShaderRead))
                usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kShaderWrite))
                usage |= VK_IMAGE_USAGE_STORAGE_BIT;

            return usage;
        }
    } // namespace


    void TranslateBufferDesc(const Core::BufferDesc bufferDesc, const Core::ResourceCommitParams params,
                             VkBufferCreateInfo& bufferCI, VmaAllocationCreateInfo& allocationCI)
    {
        const bool isTexelBuffer = bufferDesc.m_format != Core::Format::kUndefined;

        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = bufferDesc.m_size;
        bufferCI.usage = TranslateBufferUsage(params.m_bindFlags, isTexelBuffer);

        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;
        switch (params.m_memory)
        {
        default:
        case Core::ResourceMemory::kNotCommitted:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ResourceMemory::kDeviceLocal:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case Core::ResourceMemory::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case Core::ResourceMemory::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }
    }


    BufferInstance* BufferInstance::Create(const Core::BufferDesc desc, Core::ResourceCommitParams commitParams,
                                           Core::ResourcePool* pool)
    {
        auto* instance = Memory::New<BufferInstance>(&GBufferInstancePool);
        instance->m_type = Core::ResourceType::kBuffer;
        instance->m_bufferDesc = desc;
        instance->m_bindFlags = commitParams.m_bindFlags;
        instance->m_memoryStatus = commitParams.m_memory;
        instance->m_pool = pool;
        return instance;
    }


    void BufferInstance::Delete(BufferInstance* instance)
    {
        Memory::Delete(&GBufferInstancePool, instance);
    }


    void BufferInstance::Allocate(const Core::Device* device)
    {
        FE_PROFILER_ZONE();

        m_subresourceStates.push_back(Common::SubresourceState{});

        const bool isTexelBuffer = m_bufferDesc.m_format != Core::Format::kUndefined;

        const Core::ResourceCommitParams params{ .m_bindFlags = m_bindFlags, .m_memory = m_memoryStatus };

        VkBufferCreateInfo bufferCI = {};
        VmaAllocationCreateInfo allocationCI = {};
        TranslateBufferDesc(m_bufferDesc, params, bufferCI, allocationCI);

        const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();

        VerifyVk(vmaCreateBuffer(allocator, &bufferCI, &allocationCI, &m_buffer, &m_vmaAllocation, nullptr));

        if (isTexelBuffer)
        {
            VkBufferViewCreateInfo viewCI{};
            viewCI.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewCI.buffer = m_buffer;
            viewCI.format = Translate(m_bufferDesc.m_format);
            viewCI.offset = 0;
            viewCI.range = m_bufferDesc.m_size;
            VerifyVk(vkCreateBufferView(NativeCast(device), &viewCI, nullptr, &m_view));
        }
    }


    void BufferInstance::Invalidate(const Core::Device* device)
    {
        if (m_buffer)
        {
            const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
            vmaDestroyBuffer(allocator, m_buffer, m_vmaAllocation);

            if (m_view)
                vkDestroyBufferView(NativeCast(device), m_view, nullptr);

            m_vmaAllocation = nullptr;
            m_buffer = VK_NULL_HANDLE;
            m_view = VK_NULL_HANDLE;
        }
        else
        {
            FE_Assert(m_vmaAllocation == nullptr);
        }
    }


    void BufferInstance::UpdateDebugNames(Core::Device* device, const Env::Name name)
    {
        if (m_name == name)
            return;

        m_name = name;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_buffer);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(device), &nameInfo));

        if (m_view)
        {
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER_VIEW;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_view);
            nameInfo.pObjectName = m_name.c_str();
            VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(device), &nameInfo));
        }

        if (m_vmaAllocation)
        {
            vmaSetAllocationName(ImplCast(device)->GetVmaInstance(), m_vmaAllocation, m_name.c_str());
        }
    }


    void* BufferInstance::Map(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_memoryStatus == Core::ResourceMemory::kHostRandomAccess
                  || m_memoryStatus == Core::ResourceMemory::kHostWriteThrough);

        const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();

        void* result;
        VerifyVk(vmaMapMemory(allocator, m_vmaAllocation, &result));
        return result;
    }


    void BufferInstance::Unmap(Core::Device* device)
    {
        const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
        vmaUnmapMemory(allocator, m_vmaAllocation);
    }


    void BufferInstance::FlushMappedRange(Core::Device* device, const uint32_t offset, const uint32_t byteSize)
    {
        FE_Assert(m_memoryStatus == Core::ResourceMemory::kHostRandomAccess
                  || m_memoryStatus == Core::ResourceMemory::kHostWriteThrough);

        const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
        VerifyVk(vmaFlushAllocation(allocator, m_vmaAllocation, offset, byteSize));
    }


    TextureInstance* TextureInstance::Create(const Core::TextureDesc desc, const Core::ResourceCommitParams commitParams,
                                             Core::ResourcePool* pool)
    {
        auto* instance = Memory::New<TextureInstance>(&GTextureInstancePool);
        instance->m_type = Core::ResourceType::kTexture;
        instance->m_textureDesc = desc;
        instance->m_bindFlags = commitParams.m_bindFlags;
        instance->m_memoryStatus = commitParams.m_memory;
        instance->m_pool = pool;
        return instance;
    }


    void TextureInstance::Delete(TextureInstance* instance)
    {
        Memory::Delete(&GTextureInstancePool, instance);
    }


    VkImageView TextureInstance::GetSubresourceView(const Core::Device* device, const Core::TextureSubresource subresource)
    {
        FE_PROFILER_ZONE();

        std::unique_lock lk{ m_viewCacheLock };

        if (subresource == m_wholeImageSubresource)
            return m_wholeImageView;

        FE_Assert(subresource.m_firstArraySlice + subresource.m_arraySize <= m_textureDesc.m_arraySize);
        FE_Assert(subresource.m_mostDetailedMipSlice + subresource.m_mipSliceCount <= m_textureDesc.m_mipSliceCount);

        const auto it = festd::find_if(m_viewCache, [subresource](const ViewCacheEntry& entry) {
            return entry.m_subresource == subresource;
        });

        if (it != m_viewCache.end())
            return it->m_view;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_textureDesc.m_imageFormat);
        viewCI.viewType = Translate(m_textureDesc.m_dimension, m_textureDesc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_textureDesc.m_imageFormat);
        viewCI.subresourceRange.baseMipLevel = subresource.m_mostDetailedMipSlice;
        viewCI.subresourceRange.levelCount = subresource.m_mipSliceCount;
        viewCI.subresourceRange.baseArrayLayer = subresource.m_firstArraySlice;
        viewCI.subresourceRange.layerCount = subresource.m_arraySize;
        viewCI.image = m_image;

        VkImageView view = VK_NULL_HANDLE;
        VerifyVk(vkCreateImageView(NativeCast(device), &viewCI, nullptr, &view));

        const Env::Name viewName = Fmt::FormatName("{}_View", m_name);
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(view);
        nameInfo.pObjectName = viewName.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(device), &nameInfo));

        m_viewCache.push_back({ subresource, view });
        return view;
    }


    void TextureInstance::Allocate(const Core::Device* device)
    {
        FE_PROFILER_ZONE();

        m_subresourceStates.push_back(Common::SubresourceState{});

        const Core::ResourceCommitParams params{ .m_bindFlags = m_bindFlags, .m_memory = m_memoryStatus };

        VkImageCreateInfo imageCI = {};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.usage = GetImageUsage(params.m_bindFlags);

        switch (m_textureDesc.m_dimension)
        {
        case Core::TextureDimension::k1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            FE_Assert(m_textureDesc.m_height == 1);
            FE_Assert(m_textureDesc.m_depth == 1);
            break;
        case Core::TextureDimension::kCubemap:
            {
                const uint32_t arraySize = m_textureDesc.m_arraySize;
                FE_AssertMsg(arraySize == 6, "Cubemap image must have exactly 6 slices, but got {}", arraySize);
                imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                [[fallthrough]];
            }
        case Core::TextureDimension::k2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            FE_Assert(m_textureDesc.m_depth == 1);
            break;
        case Core::TextureDimension::k3D:
            imageCI.imageType = VK_IMAGE_TYPE_3D;
            break;
        default:
            FE_AssertMsg(false, "Unknown image dimension");
            break;
        }

        imageCI.extent = TranslateExtent(m_textureDesc.GetSize());
        imageCI.mipLevels = m_textureDesc.m_mipSliceCount;
        imageCI.arrayLayers = m_textureDesc.m_arraySize;
        imageCI.format = Translate(m_textureDesc.m_imageFormat);
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.samples = TranslateSampleCount(m_textureDesc.m_sampleCount);
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCI{};
        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;

        switch (params.m_memory)
        {
        default:
        case Core::ResourceMemory::kNotCommitted:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ResourceMemory::kDeviceLocal:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case Core::ResourceMemory::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            imageCI.tiling = VK_IMAGE_TILING_LINEAR;
            break;

        case Core::ResourceMemory::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            imageCI.tiling = VK_IMAGE_TILING_LINEAR;
            break;
        }

        const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();

        // TODO: maybe handle OOM differently
        VerifyVk(vmaCreateImage(allocator, &imageCI, &allocationCI, &m_image, &m_vmaAllocation, nullptr));
        InitWholeImageView(device);
    }


    void TextureInstance::InitWholeImageView(const Core::Device* device)
    {
        FE_PROFILER_ZONE();

        if (m_wholeImageView != VK_NULL_HANDLE)
            return;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_textureDesc.m_imageFormat);
        viewCI.viewType = Translate(m_textureDesc.m_dimension, m_textureDesc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_textureDesc.m_imageFormat);
        viewCI.subresourceRange.levelCount = m_textureDesc.m_mipSliceCount;
        viewCI.subresourceRange.layerCount = m_textureDesc.m_arraySize;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.image = m_image;
        VerifyVk(vkCreateImageView(NativeCast(device), &viewCI, nullptr, &m_wholeImageView));

        m_wholeImageSubresource.m_firstArraySlice = 0;
        m_wholeImageSubresource.m_mostDetailedMipSlice = 0;
        m_wholeImageSubresource.m_arraySize = m_textureDesc.m_arraySize;
        m_wholeImageSubresource.m_mipSliceCount = m_textureDesc.m_mipSliceCount;
    }


    void TextureInstance::Invalidate(const Core::Device* device)
    {
        FE_PROFILER_ZONE();

        if (m_image)
        {
            const VkDevice vkDevice = NativeCast(device);

            FE_Assert(!m_subresourceStates.empty());
            m_subresourceStates.clear();

            for (const auto& [subresource, view] : m_viewCache)
                vkDestroyImageView(vkDevice, view, nullptr);

            m_viewCache.clear();

            FE_Assert(m_wholeImageView);
            vkDestroyImageView(vkDevice, m_wholeImageView, nullptr);

            if (m_vmaAllocation)
            {
                const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
                vmaDestroyImage(allocator, m_image, m_vmaAllocation);
            }

            m_image = VK_NULL_HANDLE;
            m_memoryStatus = Core::ResourceMemory::kNotCommitted;
        }
        else
        {
            FE_Assert(m_memoryStatus == Core::ResourceMemory::kNotCommitted);
            FE_Assert(m_subresourceStates.empty());
            FE_Assert(m_viewCache.empty());
            FE_Assert(m_wholeImageView == VK_NULL_HANDLE);
            FE_Assert(m_vmaAllocation == nullptr);
        }
    }


    void TextureInstance::UpdateDebugNames(Core::Device* device, const Env::Name name)
    {
        if (m_name == name)
            return;

        m_name = name;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_image);
        nameInfo.pObjectName = name.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(device), &nameInfo));

        FE_Assert(m_wholeImageView);

        const Env::Name viewName = Fmt::FormatName("{}_View", name);
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_wholeImageView);
        nameInfo.pObjectName = viewName.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(device), &nameInfo));

        for (auto& [key, view] : m_viewCache)
        {
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(view);
            VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(device), &nameInfo));
        }

        if (m_vmaAllocation)
        {
            const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
            vmaSetAllocationName(allocator, m_vmaAllocation, name.c_str());
        }
    }


    void* TextureInstance::Map(Core::Device* device)
    {
        FE_Unused(device);
        FE_DebugBreak();
        return nullptr;
    }


    void TextureInstance::Unmap(Core::Device* device)
    {
        FE_Unused(device);
        FE_DebugBreak();
    }


    void TextureInstance::FlushMappedRange(Core::Device* device, const uint32_t offset, const uint32_t byteSize)
    {
        FE_Unused(device);
        FE_Unused(offset);
        FE_Unused(byteSize);
        FE_DebugBreak();
    }


    BufferInstance* GetInstance(const Core::Buffer* buffer)
    {
        const Common::Buffer* commonBuffer = Rtti::AssertCast<const Common::Buffer*>(buffer);
        Common::ResourceInstance* instance = commonBuffer->GetInstance();
        return Rtti::AssertCast<BufferInstance*>(instance);
    }


    VkBuffer NativeCast(const Core::Buffer* buffer)
    {
        const BufferInstance* instance = GetInstance(buffer);
        return instance->m_buffer;
    }


    TextureInstance* GetInstance(const Core::Texture* texture)
    {
        const Common::Texture* commonTexture = Rtti::AssertCast<const Common::Texture*>(texture);
        Common::ResourceInstance* instance = commonTexture->GetInstance();
        return Rtti::AssertCast<TextureInstance*>(instance);
    }


    VkImage NativeCast(const Core::Texture* texture)
    {
        const TextureInstance* instance = GetInstance(texture);
        return instance->m_image;
    }
} // namespace FE::Graphics::Vulkan
