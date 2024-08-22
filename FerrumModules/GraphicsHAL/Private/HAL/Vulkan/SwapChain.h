#pragma once
#include <HAL/SwapChain.h>
#include <HAL/Vulkan/ImageFormat.h>

namespace FE::Graphics::Vulkan
{
    class CommandQueue;
    class ImageView;

    class SwapChain final : public HAL::SwapChain
    {
        CommandQueue* m_Queue = nullptr;
        HAL::SwapChainDesc m_Desc;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_NativeSwapChain = VK_NULL_HANDLE;
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

        [[nodiscard]] bool ValidateDimensions(const HAL::SwapChainDesc& swapChainDesc) const;
        void BuildNativeSwapChain();
        void AcquireNextImage(uint32_t* index);

    public:
        FE_RTTI_Class(SwapChain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        SwapChain(HAL::Device* pDevice, HAL::Image* pDepthImage);
        ~SwapChain() override;

        HAL::ResultCode Init(const HAL::SwapChainDesc& desc) override;

        const HAL::SwapChainDesc& GetDesc() override;
        uint32_t GetCurrentImageIndex() override;
        uint32_t GetCurrentFrameIndex() override;
        uint32_t GetImageCount() override;
        Image* GetImage(uint32_t index) override;
        Image* GetCurrentImage() override;
        void Present() override;

        festd::span<HAL::ImageView*> GetRTVs() override;
        HAL::ImageView* GetDSV() override;
    };

    FE_ENABLE_IMPL_CAST(SwapChain);
} // namespace FE::Graphics::Vulkan
