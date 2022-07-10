#pragma once
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/SwapChain/ISwapChain.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKCommandQueue;
    class VKImageView;

    class VKSwapChain : public Object<ISwapChain>
    {
        VKDevice* m_Device;
        VKCommandQueue* m_Queue;
        SwapChainDesc m_Desc;

        VkSurfaceKHR m_Surface;
        VkSwapchainKHR m_NativeSwapChain;
        VkSurfaceFormatKHR m_ColorFormat;
        VkSurfaceCapabilitiesKHR m_Capabilities;

        Shared<IImage> m_DepthImage;
        Shared<IImageView> m_DepthImageView;
        List<Shared<IImage>> m_Images;
        List<Shared<IImageView>> m_ImageViews;
        UInt32 m_ImageIndex = 0;

        List<VkSemaphore> m_ImageAvailableSemaphores;
        List<VkSemaphore> m_RenderFinishedSemaphores;
        UInt32 m_FrameIndex;

        VkSemaphore* m_CurrentImageAvailableSemaphore;
        VkSemaphore* m_CurrentRenderFinishedSemaphore;

        [[nodiscard]] bool ValidateDimensions(const SwapChainDesc& swapChainDesc) const;
        void BuildNativeSwapChain(VKInstance& instance);
        void AcquireNextImage(UInt32* index);

    public:
        FE_CLASS_RTTI(VKSwapChain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        VKSwapChain(VKDevice& dev, const SwapChainDesc& desc);
        ~VKSwapChain() override;

        const SwapChainDesc& GetDesc() override;
        UInt32 GetCurrentImageIndex() override;
        UInt32 GetCurrentFrameIndex() override;
        UInt32 GetImageCount() override;
        IImage* GetImage(UInt32 index) override;
        IImage* GetCurrentImage() override;
        void Present() override;

        List<IImageView*> GetRTVs() override;
        IImageView* GetDSV() override;
    };
} // namespace FE::Osmium
