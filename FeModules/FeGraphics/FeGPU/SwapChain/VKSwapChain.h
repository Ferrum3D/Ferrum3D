#pragma once
#include <FeGPU/Image/VKImageFormat.h>
#include <FeGPU/SwapChain/ISwapChain.h>

namespace FE::GPU
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

        Vector<RefCountPtr<IImage>> m_Images;
        Vector<RefCountPtr<IImageView>> m_ImageViews;
        UInt32 m_ImageIndex = 0;

        RefCountPtr<IFence> m_Fence;

        bool ValidateDimensions(const SwapChainDesc& swapChainDesc) const;
        void BuildNativeSwapChain(VKInstance& instance);
        void AcquireNextImage(UInt32* index);

    public:
        static std::vector<vk::UniqueSemaphore> m_ImageAvailableSemaphores;
        static std::vector<vk::UniqueSemaphore> m_RenderFinishedSemaphores;
        static UInt32 m_FrameIndex;

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

        Vector<RefCountPtr<IImageView>> GetRTVs() override;
    };
} // namespace FE::GPU
