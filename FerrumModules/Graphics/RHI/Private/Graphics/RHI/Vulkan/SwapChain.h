#pragma once
#include <Graphics/RHI/ResourcePool.h>
#include <Graphics/RHI/Swapchain.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageFormat.h>

namespace FE::Graphics::Vulkan
{
    struct CommandQueue;
    struct ImageView;

    struct Swapchain final : public RHI::Swapchain
    {
        FE_RTTI_Class(Swapchain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        Swapchain(RHI::Device* device, Logger* logger, RHI::ResourcePool* resourcePool);
        ~Swapchain() override;

        VkSwapchainKHR GetNative() const
        {
            return m_nativeSwapchain;
        }

        RHI::ResultCode Init(const RHI::SwapchainDesc& desc) override;

        const RHI::SwapchainDesc& GetDesc() const override;

        void BeginFrame(const RHI::FenceSyncPoint& signalFence) override;
        void Present(const RHI::FenceSyncPoint& waitFence) override;

        uint32_t GetCurrentImageIndex() const override;
        uint32_t GetImageCount() const override;

        festd::span<RHI::ImageView* const> GetRTVs() const override;
        RHI::ImageView* GetDSV() const override;

    private:
        CommandQueue* m_queue = nullptr;
        RHI::SwapchainDesc m_desc;

        Logger* m_logger = nullptr;
        Rc<RHI::ResourcePool> m_resourcePool;

        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_nativeSwapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_colorFormat = {};
        VkSurfaceCapabilitiesKHR m_capabilities = {};

        Rc<Image> m_depthImage;
        Rc<ImageView> m_depthImageView;
        festd::small_vector<Rc<Image>, 3> m_images;
        festd::small_vector<Rc<ImageView>, 3> m_imageViews;

        festd::small_vector<VkSemaphore, 3> m_imageAvailableSemaphores;
        festd::small_vector<VkSemaphore, 3> m_renderFinishedSemaphores;
        uint32_t m_frameIndex = 0;
        uint32_t m_imageIndex = 0;

        [[nodiscard]] bool ValidateDimensions(const RHI::SwapchainDesc& swapChainDesc) const;
        void BuildNativeSwapchain();
    };

    FE_ENABLE_NATIVE_CAST(Swapchain);
} // namespace FE::Graphics::Vulkan
