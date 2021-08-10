#pragma once
#include <FeGPU/Image/VKImageFormat.h>
#include <FeGPU/SwapChain/ISwapChain.h>

namespace FE::GPU
{
    class VKDevice;
    class VKCommandQueue;

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
        UInt32 m_FrameIndex = 0;

        vk::UniqueSemaphore m_ImageAvailableSemaphore;
        vk::UniqueSemaphore m_RenderFinishedSemaphore;
        RefCountPtr<ICommandBuffer> m_CmdBuffer;
        RefCountPtr<IFence> m_Fence;

        bool ValidateDimentions(const SwapChainDesc& swapChainDesc);

    public:
        FE_CLASS_RTTI(VKSwapChain, "D8A71561-6AB2-4711-B941-0694D06D9D15");

        VKSwapChain(VKDevice& dev, const SwapChainDesc& desc);
        ~VKSwapChain();

        virtual const SwapChainDesc& GetDesc() override;
        virtual UInt32 GetCurrentImageIndex() override;
        virtual UInt32 GetImageCount() override;
        virtual IImage* GetImage(UInt32 index) override;
        virtual IImage* GetCurrentImage() override;
        virtual UInt32 NextImage(const RefCountPtr<IFence>& fence, UInt64 signal) override;
        virtual void Present(const RefCountPtr<IFence>& fence, UInt64 wait) override;
    };
} // namespace FE::GPU
