#pragma once
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/ImageFormat.h>

namespace FE::Graphics::Vulkan
{
    struct DeviceFactory;
    struct FrameGraphContext;
    struct Image;

    struct Viewport final : public Core::Viewport
    {
        static constexpr uint32_t kMaxInFlightFrames = 2;

        FE_RTTI_Class(Viewport, "1182BF45-88B6-4763-A120-BC823919D74D");

        Viewport(Core::Device* device, Logger* logger, Core::ResourcePool* resourcePool);
        ~Viewport() override;

        void Init(const Core::ViewportDesc& desc) override;
        [[nodiscard]] const Core::ViewportDesc& GetDesc() const override;

        void Present(FrameGraphContext* frameGraphContext);

        void Resize(uint32_t width, uint32_t height);

        Core::Image* GetCurrentColorTarget() override;
        Core::Image* GetDepthTarget() override;

        Core::Format GetColorTargetFormat() override;
        Core::Format GetDepthTargetFormat() override;

        void PrepareFrame();

        CommandBuffer* GetCurrentGraphicsCommandBuffer() const
        {
            return m_graphicsCommandBuffers[m_frameIndex % kMaxInFlightFrames].Get();
        }

        VkQueue GetQueue() const
        {
            return m_queue;
        }

    private:
        void CreateSurface();
        void CreateSwapChain();
        void CreateResources();

        void ReleaseResources();
        void RecreateSwapchain();

        void AcquireNextImage();

        Core::ViewportDesc m_desc;
        Core::Format m_rtvFormat = Core::Format::kUndefined;

        uint32_t m_frameIndex = 1;
        uint32_t m_imageIndex = 0;

        Logger* m_logger = nullptr;
        DeviceFactory* m_deviceFactory = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;

        VkQueue m_queue = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_colorFormat = {};
        VkSurfaceCapabilitiesKHR m_capabilities = {};

        Rc<Fence> m_fence;
        festd::small_vector<Rc<Semaphore>> m_imageAvailableSemaphores;
        festd::small_vector<Rc<Semaphore>> m_renderFinishedSemaphores;
        festd::small_vector<Rc<Image>> m_renderTargets;
        festd::small_vector<Rc<CommandBuffer>> m_graphicsCommandBuffers;

        Rc<Image> m_depthTarget;
    };

    FE_ENABLE_IMPL_CAST(Viewport);
} // namespace FE::Graphics::Vulkan
