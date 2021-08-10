#pragma once
#include <FeGPU/Fence/IFence.h>
#include <FeGPU/Image/IImage.h>

namespace FE::GPU
{
    struct SwapChainDesc
    {
        FE_STRUCT_RTTI(SwapChainDesc, "19401C0C-A89C-4393-8D40-F669AB8B128C");

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

        FE_CLASS_RTTI(ISwapChain, "B2D395D3-59B3-4552-9AC5-4B57BCB15259");

        virtual const SwapChainDesc& GetDesc()                                    = 0;
        virtual UInt32 GetCurrentImageIndex()                                     = 0;
        virtual UInt32 GetImageCount()                                            = 0;
        virtual IImage* GetImage(UInt32 index)                                    = 0;
        virtual IImage* GetCurrentImage()                                         = 0;
        virtual UInt32 NextImage(const RefCountPtr<IFence>& fence, UInt64 signal) = 0;
        virtual void Present(const RefCountPtr<IFence>& fence, UInt64 wait)       = 0;
    };
} // namespace FE::GPU
