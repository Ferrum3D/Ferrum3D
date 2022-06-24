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

        vk::UniqueSurfaceKHR m_Surface;
        vk::UniqueSwapchainKHR m_NativeSwapChain;
        vk::SurfaceFormatKHR m_ColorFormat;
        vk::SurfaceCapabilitiesKHR m_Capabilities;

        List<Shared<IImage>> m_Images;
        List<Shared<IImageView>> m_ImageViews;
        UInt32 m_ImageIndex = 0;

        Vector<vk::UniqueSemaphore> m_ImageAvailableSemaphores;
        Vector<vk::UniqueSemaphore> m_RenderFinishedSemaphores;
        UInt32 m_FrameIndex;

        vk::Semaphore* m_CurrentImageAvailableSemaphore;
        vk::Semaphore* m_CurrentRenderFinishedSemaphore;

        bool ValidateDimensions(const SwapChainDesc& swapChainDesc) const;
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

        List<Shared<IImageView>> GetRTVs() override;
    };
} // namespace FE::Osmium
