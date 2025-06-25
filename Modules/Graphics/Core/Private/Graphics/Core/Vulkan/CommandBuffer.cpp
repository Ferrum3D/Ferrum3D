#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/CommandBuffer.h>
#include <Graphics/Core/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    FE_DECLARE_VULKAN_OBJECT_POOL(CommandBuffer);


    CommandBuffer::~CommandBuffer()
    {
        if (m_nativeCommandBuffer)
        {
            vkFreeCommandBuffers(NativeCast(m_device), m_nativeCommandPool, 1, &m_nativeCommandBuffer);
        }

        m_nativeQueue = VK_NULL_HANDLE;
        m_nativeCommandBuffer = VK_NULL_HANDLE;
        m_nativeCommandPool = VK_NULL_HANDLE;
    }


    CommandBuffer* CommandBuffer::Create(Core::Device* device, const CommandBufferDesc& desc)
    {
        return Rc<CommandBuffer>::Allocate(&GCommandBufferPool, [device, &desc](void* memory) {
            return new (memory) CommandBuffer(device, desc);
        });
    }


    void CommandBuffer::Begin()
    {
        if (m_wasUsed)
        {
            VerifyVulkan(vkResetCommandBuffer(m_nativeCommandBuffer, 0));
        }

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VerifyVulkan(vkBeginCommandBuffer(m_nativeCommandBuffer, &beginInfo));
    }


    void CommandBuffer::Submit()
    {
        FE_PROFILER_ZONE();

        m_resourceBarrierBatcher.Flush();
        VerifyVulkan(vkEndCommandBuffer(m_nativeCommandBuffer));

        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;

        const uint32_t waitSemaphoreCount = m_waitSemaphores.size() + m_waitFences.size();
        const uint32_t signalSemaphoreCount = m_signalSemaphores.size() + m_signalFences.size();

        festd::pmr::vector<VkSemaphore> waitSemaphores{ &m_linearAllocator };
        festd::pmr::vector<uint64_t> waitSemaphoreValues{ &m_linearAllocator };
        festd::pmr::vector<VkPipelineStageFlags> waitStageMasks{ &m_linearAllocator };
        waitSemaphores.reserve(waitSemaphoreCount);
        waitSemaphoreValues.reserve(waitSemaphoreCount);
        waitStageMasks.reserve(waitSemaphoreCount);

        for (const auto& [fence, value] : m_waitFences)
        {
            waitSemaphores.push_back(NativeCast(fence.Get()));
            waitSemaphoreValues.push_back(value);
            waitStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

        for (const auto& [semaphore, stageMask] : m_waitSemaphores)
        {
            waitSemaphores.push_back(semaphore->GetNative());
            waitSemaphoreValues.push_back(0);
            waitStageMasks.push_back(stageMask);
        }

        festd::pmr::vector<VkSemaphore> signalSemaphores{ &m_linearAllocator };
        festd::pmr::vector<uint64_t> signalSemaphoreValues{ &m_linearAllocator };
        signalSemaphores.reserve(signalSemaphoreCount);
        signalSemaphoreValues.reserve(signalSemaphoreCount);

        for (const auto& [fence, value] : m_signalFences)
        {
            signalSemaphores.push_back(NativeCast(fence.Get()));
            signalSemaphoreValues.push_back(value);
        }

        for (const auto& semaphore : m_signalSemaphores)
        {
            signalSemaphores.push_back(semaphore->GetNative());
            signalSemaphoreValues.push_back(0);
        }

        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStageMasks.data();
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pSignalSemaphores = signalSemaphores.data();
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfo = {};
        timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

        if (!m_waitFences.empty())
        {
            timelineSemaphoreInfo.waitSemaphoreValueCount = waitSemaphoreCount;
            timelineSemaphoreInfo.pWaitSemaphoreValues = waitSemaphoreValues.data();
            submitInfo.pNext = &timelineSemaphoreInfo;
        }

        if (!m_signalFences.empty())
        {
            timelineSemaphoreInfo.signalSemaphoreValueCount = signalSemaphoreCount;
            timelineSemaphoreInfo.pSignalSemaphoreValues = signalSemaphoreValues.data();
            submitInfo.pNext = &timelineSemaphoreInfo;
        }

        submitInfo.pCommandBuffers = &m_nativeCommandBuffer;
        submitInfo.commandBufferCount = 1;

        VerifyVulkan(vkQueueSubmit(m_nativeQueue, 1, &submitInfo, nullptr));

        m_waitSemaphores.clear_and_shrink();
        m_waitFences.clear_and_shrink();
        m_signalSemaphores.clear_and_shrink();
        m_signalFences.clear_and_shrink();
        m_linearAllocator.FreeMemory();

        m_wasUsed = true;
    }


    CommandBuffer::CommandBuffer(Core::Device* device, const CommandBufferDesc& desc)
        : m_linearAllocator(4096, desc.m_pageAllocator)
        , m_nativeQueue(desc.m_queue)
        , m_nativeCommandPool(desc.m_commandPool)
        , m_resourceBarrierBatcher(device)
        , m_signalSemaphores(&m_linearAllocator)
        , m_waitSemaphores(&m_linearAllocator)
        , m_signalFences(&m_linearAllocator)
        , m_waitFences(&m_linearAllocator)
    {
        m_device = device;

        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_nativeCommandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        VerifyVulkan(vkAllocateCommandBuffers(NativeCast(device), &allocateInfo, &m_nativeCommandBuffer));
    }
} // namespace FE::Graphics::Vulkan
