#pragma once
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/SwapChain/ISwapChain.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKCommandQueue;
    class VKImageView;

    class VKSwapChain : public ISwapChain
    {
        VKDevice* m_Device;
        VKCommandQueue* m_Queue;
        SwapChainDesc m_Desc;

        VkSurfaceKHR m_Surface;
        VkSwapchainKHR m_NativeSwapChain;
        VkSurfaceFormatKHR m_ColorFormat;
        VkSurfaceCapabilitiesKHR m_Capabilities;

        Rc<IImage> m_DepthImage;
        Rc<IImageView> m_DepthImageView;
        eastl::vector<Rc<IImage>> m_Images;
        eastl::vector<Rc<IImageView>> m_ImageViews;
        uint32_t m_ImageIndex = 0;

        eastl::vector<VkSemaphore> m_ImageAvailableSemaphores;
        eastl::vector<VkSemaphore> m_RenderFinishedSemaphores;
        uint32_t m_FrameIndex = 0;

        VkSemaphore* m_CurrentImageAvailableSemaphore;
        VkSemaphore* m_CurrentRenderFinishedSemaphore;

        [[nodiscard]] bool ValidateDimensions(const SwapChainDesc& swapChainDesc) const;
        void BuildNativeSwapChain(VKInstance& instance);
        void AcquireNextImage(uint32_t* index);

    public:
        FE_RTTI_Class(VKSwapChain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        VKSwapChain(VKDevice& dev, const SwapChainDesc& desc);
        ~VKSwapChain() override;

        const SwapChainDesc& GetDesc() override;
        uint32_t GetCurrentImageIndex() override;
        uint32_t GetCurrentFrameIndex() override;
        uint32_t GetImageCount() override;
        IImage* GetImage(uint32_t index) override;
        IImage* GetCurrentImage() override;
        void Present() override;

        eastl::vector<IImageView*> GetRTVs() override;
        IImageView* GetDSV() override;
    };
} // namespace FE::Osmium
