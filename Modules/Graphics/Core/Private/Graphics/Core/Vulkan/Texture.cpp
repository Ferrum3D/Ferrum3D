#include <Graphics/Core/Vulkan/Barrier.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Format.h>
#include <Graphics/Core/Vulkan/PipelineStates.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
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


    void Texture::DecommitMemory()
    {
        if (m_instance == nullptr)
            return;

        FE_Assert(m_instance->m_pool, "Externally created textures not implemented");
        m_instance->m_pool->DecommitTextureMemory(this);
    }


    void Texture::CommitInternal(ResourcePool* resourcePool, const Core::ResourceCommitParams params)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance == nullptr);

        m_instance = TextureInstance::Create();
        m_instance->m_pool = resourcePool;
        m_instance->m_bindFlags = params.m_bindFlags;
        m_instance->m_type = m_type;

        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.usage = GetImageUsage(params.m_bindFlags);

        switch (m_desc.m_dimension)
        {
        case Core::TextureDimension::k1D:
            imageCI.imageType = VK_IMAGE_TYPE_1D;
            FE_Assert(m_desc.m_height == 1);
            FE_Assert(m_desc.m_depth == 1);
            break;
        case Core::TextureDimension::kCubemap:
            {
                const uint32_t arraySize = m_desc.m_arraySize;
                FE_AssertMsg(arraySize == 6, "Cubemap image must have exactly 6 slices, but got {}", arraySize);
                imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                [[fallthrough]];
            }
        case Core::TextureDimension::k2D:
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            FE_Assert(m_desc.m_depth == 1);
            break;
        case Core::TextureDimension::k3D:
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
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
        auto* textureInstance = Rtti::AssertCast<TextureInstance*>(m_instance);
        VerifyVk(vmaCreateImage(allocator,
                                &imageCI,
                                &allocationCI,
                                &textureInstance->m_image,
                                &textureInstance->m_vmaAllocation,
                                nullptr));

        InitWholeImageView();
        UpdateDebugNames();

        Common::SubresourceState& initialState = textureInstance->m_subresourceStates.emplace_back();
        initialState.m_value = 0;
        initialState.m_layout = Core::BarrierLayout::kUndefined;
    }


    void Texture::SwapInternal(TextureInstance*& instance)
    {
        FE_PROFILER_ZONE();

        auto* newInstance = instance;
        if (instance != nullptr)
        {
            FE_Assert(m_desc == instance->m_textureDesc);
            FE_Assert(instance->m_memoryStatus != Core::ResourceMemory::kNotCommitted);
        }

        std::unique_lock lk{ m_lock };

        for (auto& barriers : m_queueReleaseBarriers)
            FE_Assert(barriers.empty());

        auto* oldInstance = Rtti::AssertCast<TextureInstance*>(m_instance);
        m_instance = instance;
        instance = oldInstance;

        if (newInstance->m_wholeImageView == VK_NULL_HANDLE)
            InitWholeImageView();

        UpdateDebugNames();
    }


    VkImageView Texture::GetSubresourceView(const Core::TextureSubresource subresource) const
    {
        FE_PROFILER_ZONE();

        std::unique_lock lk{ m_lock };

        auto* textureInstance = Rtti::AssertCast<TextureInstance*>(m_instance);

        if (subresource == m_wholeImageSubresource)
            return textureInstance->m_wholeImageView;

        FE_Assert(subresource.m_firstArraySlice + subresource.m_arraySize <= m_desc.m_arraySize);
        FE_Assert(subresource.m_mostDetailedMipSlice + subresource.m_mipSliceCount <= m_desc.m_mipSliceCount);

        const auto it = festd::find_if(textureInstance->m_viewCache, [subresource](const TextureInstance::ViewCacheEntry& entry) {
            return entry.m_subresource == subresource;
        });
        if (it != textureInstance->m_viewCache.end())
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
        viewCI.image = textureInstance->m_image;

        VkImageView view = VK_NULL_HANDLE;
        VerifyVk(vkCreateImageView(NativeCast(m_device), &viewCI, nullptr, &view));

        const Env::Name viewName = Fmt::FormatName("{}_View", m_name);
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(view);
        nameInfo.pObjectName = viewName.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        textureInstance->m_viewCache.push_back({ subresource, view });
        return view;
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

        auto* textureInstance = Rtti::AssertCast<TextureInstance*>(m_instance);

        FE_Assert(textureInstance->m_wholeImageView == VK_NULL_HANDLE);

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.format = Translate(m_desc.m_imageFormat);
        viewCI.viewType = Translate(m_desc.m_dimension, m_desc.m_arraySize > 1);
        viewCI.subresourceRange.aspectMask = TranslateImageAspectFlags(m_desc.m_imageFormat);
        viewCI.subresourceRange.levelCount = m_desc.m_mipSliceCount;
        viewCI.subresourceRange.layerCount = m_desc.m_arraySize;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.image = textureInstance->m_image;
        VerifyVk(vkCreateImageView(NativeCast(m_device), &viewCI, nullptr, &textureInstance->m_wholeImageView));

        m_wholeImageSubresource.m_firstArraySlice = 0;
        m_wholeImageSubresource.m_mostDetailedMipSlice = 0;
        m_wholeImageSubresource.m_arraySize = m_desc.m_arraySize;
        m_wholeImageSubresource.m_mipSliceCount = m_desc.m_mipSliceCount;
    }


    void Texture::UpdateDebugNames()
    {
        if (m_instance == nullptr)
            return;

        auto* textureInstance = Rtti::AssertCast<TextureInstance*>(m_instance);

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(textureInstance->m_image);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        FE_Assert(textureInstance->m_wholeImageView);

        const Env::Name viewName = Fmt::FormatName("{}_View", m_name);
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(textureInstance->m_wholeImageView);
        nameInfo.pObjectName = viewName.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        for (auto& [key, view] : textureInstance->m_viewCache)
        {
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(view);
            VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));
        }

        if (textureInstance->m_vmaAllocation)
        {
            const VmaAllocator allocator = ImplCast(textureInstance->m_pool)->GetAllocator();
            vmaSetAllocationName(allocator, textureInstance->m_vmaAllocation, m_name.c_str());
        }
    }
} // namespace FE::Graphics::Vulkan
