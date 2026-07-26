#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    ResourcePool::ResourcePool(Core::Device* device, Core::GraphicsQueue* graphicsQueue, Core::AsyncCopyQueue* asyncCopyQueue)
    {
        m_device = device;
        m_graphicsQueue = ImplCast(graphicsQueue);
        m_asyncCopyQueue = ImplCast(asyncCopyQueue);

        SetImmediateDestroyPolicy();
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
    }


    template<class TDesc, class TParams>
    uint32_t ResourcePool::FindFreeResource(const TDesc& desc, const TParams& params)
    {
        uint32_t bestCompatibilityScore = 0;
        uint32_t bestResourceIndex = kInvalidIndex;
        Bit::Traverse(m_freedResources.view(), [&](const uint32_t resourceIndex) {
            const Common::ResourceInstance* resource = m_resources[resourceIndex];
            const uint32_t compatibilityScore = resource->ScoreCompatibility(desc, params);
            if (compatibilityScore > bestCompatibilityScore)
            {
                bestCompatibilityScore = compatibilityScore;
                bestResourceIndex = resourceIndex;
            }
        });

        return bestResourceIndex;
    }


    void ResourcePool::CommitBufferMemory(Core::Buffer* buffer, const Core::ResourceCommitParams& params)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(buffer->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted);

        Common::ResourceInstance* instance;
        const uint32_t bestResourceIndex = FindFreeResource(buffer->GetDesc(), params);
        if (bestResourceIndex != kInvalidIndex)
        {
            instance = m_resources[bestResourceIndex];
            FE_Assert(instance);

            m_resources[bestResourceIndex] = nullptr;
            m_emptyResources.set(bestResourceIndex);
        }
        else
        {
            auto* newInstance = BufferInstance::Create(buffer->GetDesc(), params, this);
            newInstance->Allocate(m_device);
            instance = newInstance;
        }

        Common::ImplCast(buffer)->AssignInstance(instance);
    }


    void ResourcePool::CommitTextureMemory(Core::Texture* texture, const Core::ResourceCommitParams& params)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(texture->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted);

        Common::ResourceInstance* instance;
        const uint32_t bestResourceIndex = FindFreeResource(texture->GetDesc(), params);
        if (bestResourceIndex != kInvalidIndex)
        {
            instance = m_resources[bestResourceIndex];
            FE_Assert(instance);

            m_resources[bestResourceIndex] = nullptr;
            m_emptyResources.set(bestResourceIndex);
        }
        else
        {
            auto* newInstance = TextureInstance::Create(texture->GetDesc(), params, this);
            newInstance->Allocate(m_device);
            instance = newInstance;
        }

        Common::ImplCast(texture)->AssignInstance(instance);
    }


    void ResourcePool::DecommitBufferMemory(Core::Buffer* buffer)
    {
        std::unique_lock lk{ m_lock };

        Common::ResourceInstance* instance = nullptr;
        Common::ImplCast(buffer)->SwapInstance(instance);
        FinalizeDecommit(instance);
    }


    void ResourcePool::DecommitTextureMemory(Core::Texture* texture)
    {
        std::unique_lock lk{ m_lock };

        Common::ResourceInstance* instance = nullptr;
        Common::ImplCast(texture)->SwapInstance(instance);
        FinalizeDecommit(instance);
    }


    void ResourcePool::EndFrame()
    {
        std::unique_lock lk{ m_lock };

        Bit::Traverse(m_pendingResources.view(), [&](const uint32_t resourceIndex) {
            const Common::ResourceInstance* resource = m_resources[resourceIndex];
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


    void ResourcePool::FinalizeDecommit(Common::ResourceInstance* resourceInstance)
    {
        const uint64_t graphicsQueueFenceValue = m_graphicsQueue->GetCurrentFence().m_value;
        const uint64_t transferQueueFenceValue = m_asyncCopyQueue->GetCurrentFence().m_value;
        resourceInstance->m_lastFenceValues[festd::to_underlying(Core::DeviceQueueType::kGraphics)] = graphicsQueueFenceValue;
        resourceInstance->m_lastFenceValues[festd::to_underlying(Core::DeviceQueueType::kTransfer)] = transferQueueFenceValue;

        const uint32_t slot = AllocateResourceSlot();
        m_resources[slot] = resourceInstance;
        m_pendingResources.set(slot);
    }
} // namespace FE::Graphics::Vulkan
