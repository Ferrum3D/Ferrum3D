#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Image.h>
#include <Graphics/Core/Vulkan/ImageFormat.h>
#include <Graphics/Core/Vulkan/PipelineStates.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>
#include <Graphics/Core/Vulkan/Texture.h>

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


        VkImageLayout Translate(const Core::BarrierLayout layout)
        {
            switch (layout)
            {
            default:
                FE_DebugBreak();
                [[fallthrough]];
            case Core::BarrierLayout::kUndefined:
                return VK_IMAGE_LAYOUT_UNDEFINED;
            case Core::BarrierLayout::kRenderTarget:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case Core::BarrierLayout::kShaderRead:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case Core::BarrierLayout::kShaderReadWrite:
                return VK_IMAGE_LAYOUT_GENERAL;
            case Core::BarrierLayout::kDepthStencilRead:
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            case Core::BarrierLayout::kDepthStencilWrite:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case Core::BarrierLayout::kCopySource:
            case Core::BarrierLayout::kResolveSource:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case Core::BarrierLayout::kCopyDest:
            case Core::BarrierLayout::kResolveDest:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case Core::BarrierLayout::kShadingRateSource:
                return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
            case Core::BarrierLayout::kPresentSource:
                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }
        }
    } // namespace


    FE_DECLARE_VULKAN_OBJECT_POOL(Texture);


    Texture::~Texture()
    {
        DecommitMemory();
    }


    Texture* Texture::Create(Core::Device* device, Env::Name name, const Core::TextureDesc& desc)
    {
        FE_PROFILER_ZONE();
        return Rc<Texture>::Allocate(&GTexturePool, [device, name, &desc](void* memory) {
            return new (memory) Texture(device, name, desc);
        });
    }


    void Texture::CommitInternal(ResourcePool* resourcePool, const Core::TextureCommitParams params)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance == nullptr);

        m_instance = TextureInstance::Create();
        m_instance->m_pool = resourcePool;

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.usage = GetImageUsage(params.m_bindFlags);

        switch (m_desc.m_dimension)
        {
        case Core::ImageDimension::k1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            FE_Assert(m_desc.m_height == 1);
            FE_Assert(m_desc.m_depth == 1);
            break;
        case Core::ImageDimension::kCubemap:
            FE_AssertMsg(m_desc.m_arraySize == 6, "Cubemap image must have exactly 6 slices, but got {}", m_desc.m_arraySize);
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            [[fallthrough]];
        case Core::ImageDimension::k2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            FE_Assert(m_desc.m_depth == 1);
            break;
        case Core::ImageDimension::k3D:
            imageCI.imageType = VK_IMAGE_TYPE_3D;
            break;
        default:
            FE_AssertMsg(false, "Unknown image dimension");
            break;
        }

        imageCI.extent = TranslateExtent(m_desc.GetSize());
        imageCI.mipLevels = m_desc.m_mipSliceCount;
        imageCI.arrayLayers = m_desc.m_arraySize;
        imageCI.format = Translate(m_desc.m_imageFormat);
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = Translate(params.m_initialLayout);
        imageCI.samples = GetVKSampleCountFlags(m_desc.m_sampleCount);
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

        m_instance->m_memoryStatus = params.m_memory;

        const VmaAllocator allocator = ImplCast(m_instance->m_pool)->GetAllocator();

        // TODO: maybe handle OOM differently
        VerifyVk(vmaCreateImage(allocator, &imageCI, &allocationCI, &m_instance->m_image, &m_instance->m_vmaAllocation, nullptr));

        InitWholeImageView();
        UpdateDebugNames();

        Common::SubresourceState& initialState = m_instance->m_subresourceStates.emplace_back();
        initialState.m_value = 0;
        initialState.m_layout = params.m_initialLayout;
    }


    void Texture::SwapInternal(TextureInstance*& instance)
    {
        FE_PROFILER_ZONE();

        if (instance != nullptr)
        {
            FE_Assert(m_desc == instance->m_textureDesc);
            FE_Assert(instance->m_memoryStatus != Core::ResourceMemory::kNotCommitted);
        }

        std::unique_lock lk{ m_lock };

        for (auto& barriers : m_queueReleaseBarriers)
            FE_Assert(barriers.empty());

        festd::swap(m_instance, instance);

        UpdateDebugNames();
    }


    void Texture::DecommitMemory()
    {
        if (m_instance == nullptr)
            return;

        FE_Assert(m_instance->m_pool, "Externally created textures not implemented");
        m_instance->m_pool->DecommitTextureMemory(this);
    }


    Core::ResourceMemory Texture::GetMemoryStatus() const
    {
        if (m_instance == nullptr)
            return Core::ResourceMemory::kNotCommitted;

        return m_instance->m_memoryStatus;
    }


    VkImageView Texture::GetSubresourceView(const Core::TextureSubresource subresource) const
    {
        FE_PROFILER_ZONE();

        std::unique_lock lk{ m_lock };

        if (subresource == m_wholeImageSubresource)
            return m_instance->m_wholeImageView;

        FE_Assert(subresource.m_firstArraySlice + subresource.m_arraySize <= m_desc.m_arraySize);
        FE_Assert(subresource.m_mostDetailedMipSlice + subresource.m_mipSliceCount <= m_desc.m_mipSliceCount);

        const auto it = m_instance->m_viewCache.find(subresource);
        if (it != m_instance->m_viewCache.end())
            return it->second;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_desc.m_imageFormat);
        viewCI.viewType = Translate(m_desc.m_dimension, m_desc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.baseMipLevel = subresource.m_mostDetailedMipSlice;
        viewCI.subresourceRange.levelCount = subresource.m_mipSliceCount;
        viewCI.subresourceRange.baseArrayLayer = subresource.m_firstArraySlice;
        viewCI.subresourceRange.layerCount = subresource.m_arraySize;
        viewCI.image = m_instance->m_image;

        VkImageView view = VK_NULL_HANDLE;
        VerifyVk(vkCreateImageView(NativeCast(m_device), &viewCI, nullptr, &view));

        const Env::Name viewName = Fmt::FormatName("{}_View", m_name);
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(view);
        nameInfo.pObjectName = viewName.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        m_instance->m_viewCache[subresource] = view;
        return view;
    }


    void Texture::SetState(const Core::TextureSubresource subresource, const Common::SubresourceState state)
    {
        std::unique_lock lk{ m_lock };
        SetStateNoLock(subresource, state);
    }


    void Texture::SetStateNoLock(const Core::TextureSubresource subresource, const Common::SubresourceState state) const
    {
        FE_Assert(m_instance);

        auto& subresourceStates = m_instance->m_subresourceStates;
        if (subresourceStates.size() == 1)
        {
            if (subresource == m_wholeImageSubresource)
            {
                subresourceStates[0] = state;
                return;
            }

            subresourceStates.resize(m_desc.m_mipSliceCount * m_desc.m_arraySize);
        }

        const Core::ImageSubresourceIterator subresourceIterator{ subresource };
        for (const auto [mipIndex, arrayIndex] : subresourceIterator)
            subresourceStates[mipIndex * m_desc.m_arraySize + arrayIndex] = state;
    }


    Common::SubresourceState Texture::GetState(const Core::TextureSubresource subresource) const
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(m_instance);

        FE_AssertDebug(subresource.m_mipSliceCount == 1);
        FE_AssertDebug(subresource.m_arraySize == 1);

        auto& subresourceStates = m_instance->m_subresourceStates;
        if (subresourceStates.size() == 1)
            return subresourceStates[0];

        return subresourceStates[subresource.m_mostDetailedMipSlice * m_desc.m_arraySize + subresource.m_firstArraySlice];
    }


    void Texture::AddQueueReleaseBarrier(const Core::TextureSubresource subresource, const Common::SubresourceState state,
                                         const Core::DeviceQueueType receiverQueue)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(state.m_queueType != receiverQueue);

        SetStateNoLock(subresource, state);
        m_queueReleaseBarriers[festd::to_underlying(receiverQueue)].push_back(subresource);
    }


    festd::pmr::vector<Core::TextureSubresource> Texture::RetrieveQueueReleaseBarriers(const Core::DeviceQueueType receiverQueue,
                                                                                       std::pmr::memory_resource* allocator)
    {
        std::unique_lock lk{ m_lock };

        auto& barriers = m_queueReleaseBarriers[festd::to_underlying(receiverQueue)];
        festd::pmr::vector<Core::TextureSubresource> result{ allocator };
        result.reserve(barriers.size());
        festd::copy(barriers.begin(), barriers.end(), festd::back_inserter(result));
        barriers.clear();
        return result;
    }


    Texture::Texture(Core::Device* device, const Env::Name name, const Core::TextureDesc& desc)
    {
        m_device = device;
        m_name = name;
        m_desc = desc;
        m_type = Core::ResourceType::kTexture;
        Register();
    }


    void Texture::InitWholeImageView()
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance->m_wholeImageView == nullptr);

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_desc.m_imageFormat);
        viewCI.viewType = Translate(m_desc.m_dimension, m_desc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.levelCount = m_desc.m_mipSliceCount;
        viewCI.subresourceRange.layerCount = m_desc.m_arraySize;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.image = m_instance->m_image;
        VerifyVk(vkCreateImageView(NativeCast(m_device), &viewCI, nullptr, &m_instance->m_wholeImageView));

        m_wholeImageSubresource.m_firstArraySlice = 0;
        m_wholeImageSubresource.m_mostDetailedMipSlice = 0;
        m_wholeImageSubresource.m_arraySize = m_desc.m_arraySize;
        m_wholeImageSubresource.m_mipSliceCount = m_desc.m_mipSliceCount;
    }


    void Texture::UpdateDebugNames()
    {
        if (m_instance == nullptr)
            return;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_instance->m_image);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        FE_Assert(m_instance->m_wholeImageView);

        const Env::Name viewName = Fmt::FormatName("{}_View", m_name);
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_instance->m_wholeImageView);
        nameInfo.pObjectName = viewName.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        for (auto& [key, view] : m_instance->m_viewCache)
        {
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(view);
            VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));
        }

        if (m_instance->m_vmaAllocation)
        {
            const VmaAllocator allocator = ImplCast(m_instance->m_pool)->GetAllocator();
            vmaSetAllocationName(allocator, m_instance->m_vmaAllocation, m_name.c_str());
        }
    }
} // namespace FE::Graphics::Vulkan
