#pragma once
#include <FeGPU/Fence/IFence.h>
#include <FeGPU/Image/IImage.h>

namespace FE::GPU
{
    struct SwapChainDesc
    {
        UInt32 ImageCount = 3;
        UInt32 ImageWidth;
        UInt32 ImageHeight;
        bool VerticalSync = false;
        ICommandQueue* Queue;

        void* NativeWindowHandle = nullptr;
    };

    class ISwapChain : public IObject
    {
    public:
        virtual ~ISwapChain() = default;

        virtual const SwapChainDesc& GetDesc()                                        = 0;
        virtual UInt32 GetCurrentImageIndex()                                       = 0;
        virtual UInt32 GetImageCount()                                              = 0;
        virtual IImage* GetImage(UInt32 index)                                      = 0;
        virtual IImage* GetCurrentImage()                                             = 0;
        virtual UInt32 NextImage(const RefCountPtr<IFence>& fence, UInt64 signal) = 0;
        virtual void Present(const RefCountPtr<IFence>& fence, UInt64 wait)         = 0;
    };
} // namespace FE::GPU
