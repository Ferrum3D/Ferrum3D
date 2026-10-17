#pragma once
#include <Core/Containers/SegmentedVector.h>
#include <Graphics/Core/BaseTypes.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <tracy/TracyVulkan.hpp>

namespace FE::Graphics::Vulkan
{
    struct GpuProfiler final
    {
        ~GpuProfiler();

        void Init(Core::Device* device, Core::DeviceQueueType queueType, VkQueue queue, const char* name);
        void Shutdown();
        void BeginZone(VkCommandBuffer commandBuffer, const char* name, SourceLocation location = SourceLocation::Current());
        void EndZone();
        void Collect(VkCommandBuffer commandBuffer);
        //! All profiled submissions must have completed; query resets are submitted without waiting on the CPU.
        void CollectIdle();

    private:
#if defined(TRACY_ENABLE)
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_queue = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_collectionCommands = VK_NULL_HANDLE;
        VkFence m_collectionFence = VK_NULL_HANDLE;
        TracyVkCtx m_context = nullptr;
        SegmentedVector<tracy::VkCtxScope> m_zones;
        uint32_t m_remainingHostQueries = 0;
        bool m_usesHostQueryReset = false;
        bool m_hasUncollectedZones = false;
#endif
    };
} // namespace FE::Graphics::Vulkan
