#pragma once
#include <HAL/Swapchain.h>
#include <HAL/Vulkan/ImageFormat.h>

namespace FE::Graphics::Vulkan
{
    struct CommandQueue;
    class ImageView;

    class Swapchain final : public HAL::Swapchain
    {
        CommandQueue* m_Queue = nullptr;
        HAL::SwapchainDesc m_Desc;

        Logger* m_logger = nullptr;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_NativeSwapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_ColorFormat;
        VkSurfaceCapabilitiesKHR m_Capabilities;

        Rc<Image> m_DepthImage;
        Rc<ImageView> m_DepthImageView;
        festd::small_vector<Rc<Image>, 3> m_Images;
        festd::small_vector<Rc<ImageView>, 3> m_ImageViews;

        festd::small_vector<VkSemaphore, 3> m_ImageAvailableSemaphores;
        festd::small_vector<VkSemaphore, 3> m_RenderFinishedSemaphores;
        uint32_t m_FrameIndex = 0;
        uint32_t m_ImageIndex = 0;

        [[nodiscard]] bool ValidateDimensions(const HAL::SwapchainDesc& swapChainDesc) const;
        void BuildNativeSwapchain();

    public:
        FE_RTTI_Class(Swapchain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        Swapchain(HAL::Device* pDevice, Logger* logger, HAL::Image* pDepthImage);
        ~Swapchain() override;

        VkSwapchainKHR GetNative() const
        {
            return m_NativeSwapchain;
        }

        HAL::ResultCode Init(const HAL::SwapchainDesc& desc) override;

        const HAL::SwapchainDesc& GetDesc() const override;

        void BeginFrame(const HAL::FenceSyncPoint& signalFence) override;
        void Present(const HAL::FenceSyncPoint& waitFence) override;

        uint32_t GetCurrentImageIndex() const override;
        uint32_t GetImageCount() const override;

        festd::span<HAL::ImageView* const> GetRTVs() const override;
        HAL::ImageView* GetDSV() const override;
    };

    FE_ENABLE_NATIVE_CAST(Swapchain);
} // namespace FE::Graphics::Vulkan
