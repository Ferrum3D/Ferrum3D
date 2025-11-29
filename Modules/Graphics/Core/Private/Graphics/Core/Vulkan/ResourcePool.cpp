#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    ResourcePool::ResourcePool(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        m_device = device;
        SetImmediateDestroyPolicy();

        VmaAllocatorCreateInfo createInfo = {};
        createInfo.device = NativeCast(device);
        createInfo.physicalDevice = ImplCast(device)->GetNativeAdapter();
        createInfo.instance = NativeCast(ImplCast(device)->GetDeviceFactory());
        createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        VerifyVk(vmaCreateAllocator(&createInfo, &m_vmaAllocator));
    }


    ResourcePool::~ResourcePool()
    {
        m_device->WaitIdle();

        if (Build::IsDebug())
        {
            festd::vector<const Core::Resource*> resources;
            festd::vector<const char*> names;
            for (const Core::Resource& resource : ImplCast(m_device)->GetResources())
            {
                resources.push_back(&resource);
                names.push_back(resource.GetName().c_str());
            }

            if (!resources.empty())
                FE_DebugBreak();
        }

        vmaDestroyAllocator(m_vmaAllocator);
    }


    Core::Texture* ResourcePool::CreateTexture(const Env::Name name, const Core::TextureDesc desc)
    {
        return Texture::Create(m_device, name, desc);
    }


    Core::Buffer* ResourcePool::CreateBuffer(const Env::Name name, const Core::BufferDesc desc)
    {
        return Buffer::Create(m_device, name, desc);
    }


    template<class TDesc, class TParams>
    uint32_t ResourcePool::FindFreeResource(const TDesc& desc, const TParams& params)
    {
        uint32_t bestCompatibilityScore = 0;
        uint32_t bestResourceIndex = kInvalidIndex;
        Bit::Traverse(m_freedResources.view(), [&](const uint32_t resourceIndex) {
            const ResourceInstance* resource = m_resources[resourceIndex];
            const uint32_t compatibilityScore = resource->ScoreCompatibility(desc, params);
            if (compatibilityScore > bestCompatibilityScore)
            {
                bestCompatibilityScore = compatibilityScore;
                bestResourceIndex = resourceIndex;
            }
        });

        return bestResourceIndex;
    }


    void ResourcePool::CommitBufferMemory(Core::Buffer* buffer, const Core::BufferCommitParams& params)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(buffer->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted);

        const uint32_t bestResourceIndex = FindFreeResource(buffer->GetDesc(), params);
        if (bestResourceIndex != kInvalidIndex)
        {
            ResourceInstance* bestResource = m_resources[bestResourceIndex];
            FE_Assert(bestResource);

            m_resources[bestResourceIndex] = nullptr;
            m_emptyResources.set(bestResourceIndex);
            auto* bufferInstance = RTTI::AssertCast<BufferInstance*>(bestResource);
            ImplCast(buffer)->SwapInternal(bufferInstance);
            return;
        }

        ImplCast(buffer)->CommitInternal(this, params);
    }


    void ResourcePool::CommitTextureMemory(Core::Texture* texture, const Core::TextureCommitParams& params)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(texture->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted);

        const uint32_t bestResourceIndex = FindFreeResource(texture->GetDesc(), params);
        if (bestResourceIndex != kInvalidIndex)
        {
            ResourceInstance* bestResource = m_resources[bestResourceIndex];
            FE_Assert(bestResource);

            m_resources[bestResourceIndex] = nullptr;
            m_emptyResources.set(bestResourceIndex);

            auto* textureInstance = RTTI::AssertCast<TextureInstance*>(bestResource);
            ImplCast(texture)->SwapInternal(textureInstance);
            FE_Assert(textureInstance == nullptr);
            return;
        }

        ImplCast(texture)->CommitInternal(this, params);
    }


    void ResourcePool::DecommitBufferMemory(Core::Buffer* buffer)
    {
        std::unique_lock lk{ m_lock };

        const uint32_t slot = AllocateResourceSlot();
        BufferInstance* instance = nullptr;
        ImplCast(buffer)->SwapInternal(instance);
        m_resources[slot] = instance;
        m_pendingResources.set(slot);
    }


    void ResourcePool::DecommitTextureMemory(Core::Texture* texture)
    {
        std::unique_lock lk{ m_lock };

        const uint32_t slot = AllocateResourceSlot();
        TextureInstance* instance = nullptr;
        ImplCast(texture)->SwapInternal(instance);
        m_resources[slot] = instance;
        m_pendingResources.set(slot);
    }


    void ResourcePool::EndFrame()
    {
        std::unique_lock lk{ m_lock };

        Bit::Traverse(m_pendingResources.view(), [&](const uint32_t resourceIndex) {
            const ResourceInstance* resource = m_resources[resourceIndex];
            FE_Assert(resource);

            m_freedResources.set(resourceIndex);
        });

        m_pendingResources.reset();
    }


    uint32_t ResourcePool::AllocateResourceSlot()
    {
        uint32_t emptySlot = m_emptyResources.find_first();
        if (emptySlot == kInvalidIndex)
        {
            constexpr uint32_t kGrowSize = 1024;

            emptySlot = m_resources.size();
            m_resources.resize(m_resources.size() + kGrowSize);
            m_emptyResources.resize(m_emptyResources.size() + kGrowSize, true);
            m_freedResources.resize(m_freedResources.size() + kGrowSize, false);
            m_pendingResources.resize(m_pendingResources.size() + kGrowSize, false);
        }

        FE_Assert(!m_freedResources.test(emptySlot));
        FE_Assert(!m_pendingResources.test(emptySlot));
        FE_Assert(m_resources[emptySlot] == nullptr);
        m_emptyResources.reset(emptySlot);
        return emptySlot;
    }
} // namespace FE::Graphics::Vulkan
