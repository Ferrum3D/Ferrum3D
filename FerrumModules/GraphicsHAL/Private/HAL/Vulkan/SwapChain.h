#pragma once
#include <HAL/Swapchain.h>
#include <HAL/Vulkan/ImageFormat.h>

namespace FE::Graphics::Vulkan
{
    class CommandQueue;
    class ImageView;

    class Swapchain final : public HAL::Swapchain
    {
        CommandQueue* m_Queue = nullptr;
        HAL::SwapchainDesc m_Desc;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_NativeSwapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_ColorFormat;
        VkSurfaceCapabilitiesKHR m_Capabilities;

        Rc<Image> m_DepthImage;
        Rc<ImageView> m_DepthImageView;
        festd::vector<Rc<Image>> m_Images;
        festd::vector<Rc<ImageView>> m_ImageViews;
        uint32_t m_ImageIndex = 0;

        eastl::vector<VkSemaphore> m_ImageAvailableSemaphores;
        eastl::vector<VkSemaphore> m_RenderFinishedSemaphores;
        uint32_t m_FrameIndex = 0;

        VkSemaphore* m_CurrentImageAvailableSemaphore = nullptr;
        VkSemaphore* m_CurrentRenderFinishedSemaphore = nullptr;

        [[nodiscard]] bool ValidateDimensions(const HAL::SwapchainDesc& swapChainDesc) const;
        void BuildNativeSwapchain();
        void AcquireNextImage(uint32_t* index);

    public:
        FE_RTTI_Class(Swapchain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        Swapchain(HAL::Device* pDevice, HAL::Image* pDepthImage);
        ~Swapchain() override;

        inline VkSwapchainKHR GetNative() const
        {
            return m_NativeSwapchain;
        }

        HAL::ResultCode Init(const HAL::SwapchainDesc& desc) override;

        const HAL::SwapchainDesc& GetDesc() override;
        uint32_t GetCurrentImageIndex() override;
        uint32_t GetCurrentFrameIndex() override;
        uint32_t GetImageCount() override;
        Image* GetImage(uint32_t index) override;
        Image* GetCurrentImage() override;
        void Present() override;

        festd::span<HAL::ImageView*> GetRTVs() override;
        HAL::ImageView* GetDSV() override;
    };

    FE_ENABLE_IMPL_CAST(Swapchain);
} // namespace FE::Graphics::Vulkan
