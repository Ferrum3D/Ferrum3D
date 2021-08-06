#pragma once
#include <FeGPU/Fence/IFence.h>
#include <FeGPU/Image/IImage.h>

namespace FE::GPU
{
    struct SwapChainDesc
    {
        uint32_t ImageCount = 3;
        uint32_t ImageWidth;
        uint32_t ImageHeight;
        bool VerticalSync = false;
        ICommandQueue* Queue;

        void* NativeWindowHandle = nullptr;
    };

    class ISwapChain
    {
    public:
        virtual ~ISwapChain() = default;

        virtual const SwapChainDesc& GetDesc()                                        = 0;
        virtual uint32_t GetCurrentImageIndex()                                       = 0;
        virtual uint32_t GetImageCount()                                              = 0;
        virtual IImage* GetImage(uint32_t index)                                      = 0;
        virtual IImage* GetCurrentImage()                                             = 0;
        virtual uint32_t NextImage(const RefCountPtr<IFence>& fence, uint64_t signal) = 0;
        virtual void Present(const RefCountPtr<IFence>& fence, uint64_t wait)         = 0;
    };
} // namespace FE::GPU
