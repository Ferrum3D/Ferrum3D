#pragma once
#include <FeGPU/SwapChain/ISwapChain.h>
#include <FeGPU/Image/VKImageFormat.h>

namespace FE::GPU
{
    class VKDevice;
    class VKCommandQueue;

    class VKSwapChain : public ISwapChain
    {
        VKDevice* m_Device;
        VKCommandQueue* m_Queue;
        SwapChainDesc m_Desc;

        vk::UniqueSurfaceKHR m_Surface;
        vk::UniqueSwapchainKHR m_NativeSwapChain;
        vk::SurfaceFormatKHR m_ColorFormat;
        vk::SurfaceCapabilitiesKHR m_Capabilities;

        Vector<RefCountPtr<IImage>> m_Images;
        uint32_t m_FrameIndex = 0;

        vk::UniqueSemaphore m_ImageAvailableSemaphore;
        vk::UniqueSemaphore m_RenderFinishedSemaphore;
        RefCountPtr<ICommandBuffer> m_CmdBuffer;
        RefCountPtr<IFence> m_Fence;

        bool ValidateDimentions(const SwapChainDesc& swapChainDesc);

    public:
        VKSwapChain(VKDevice& dev, const SwapChainDesc& desc);
        ~VKSwapChain();

        virtual const SwapChainDesc& GetDesc() override;
        virtual uint32_t GetCurrentImageIndex() override;
        virtual uint32_t GetImageCount() override;
        virtual IImage* GetImage(uint32_t index) override;
        virtual IImage* GetCurrentImage() override;
        virtual uint32_t NextImage(const RefCountPtr<IFence>& fence, uint64_t signal) override;
        virtual void Present(const RefCountPtr<IFence>& fence, uint64_t wait) override;
    };
}
