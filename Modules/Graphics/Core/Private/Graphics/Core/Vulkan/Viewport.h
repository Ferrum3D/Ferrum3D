#pragma once
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Format.h>

namespace FE::Graphics::Vulkan
{
    struct DeviceFactory;
    struct CommandBuffer;
    struct GraphicsQueue;

    struct Viewport final : public Core::Viewport
    {
        FE_RTTI("1182BF45-88B6-4763-A120-BC823919D74D");

        Viewport(Core::Device* device, Logger* logger, Core::ResourcePool* resourcePool, Core::GraphicsQueue* commandQueue);
        ~Viewport() override;

        void Init(const Core::ViewportDesc& desc) override;
        [[nodiscard]] const Core::ViewportDesc& GetDesc() const override;

        void Present() override;

        void Resize(uint32_t width, uint32_t height);

        Core::Texture* GetCurrentColorTarget() override;
        void AcquireNextImage() override;

    private:
        void Present(CommandBuffer* commandBuffer);

        void CreateSurface();
        void CreateSwapChain();
        void CreateResources();

        void ReleaseResources();
        void RecreateSwapchain();

        Core::ViewportDesc m_desc;
        Core::Format m_rtvFormat = Core::Format::kUndefined;

        uint32_t m_imageIndex = 0;

        Logger* m_logger = nullptr;
        DeviceFactory* m_deviceFactory = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;
        GraphicsQueue* m_commandQueue = nullptr;

        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_colorFormat = {};
        VkSurfaceCapabilitiesKHR m_capabilities = {};

        festd::inline_vector<Rc<Semaphore>> m_imageAvailableSemaphores;
        festd::inline_vector<Rc<Semaphore>> m_renderFinishedSemaphores;

        festd::inline_vector<Rc<Texture>> m_images;
        festd::inline_vector<TextureInstance*> m_imageInstances;
    };

    FE_ENABLE_IMPL_CAST(Viewport);
} // namespace FE::Graphics::Vulkan
