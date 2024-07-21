#pragma once
#include <OsGPU/Image/ImageFormat.h>

namespace FE::Osmium
{
    class ICommandQueue;

    struct SwapChainDesc
    {
        FE_STRUCT_RTTI(SwapChainDesc, "19401C0C-A89C-4393-8D40-F669AB8B128C");

        UInt32 ImageCount = 3;
        UInt32 FrameCount = 2;
        UInt32 ImageWidth = 0;
        UInt32 ImageHeight = 0;
        bool VerticalSync = false;
        Format Format = Format::None;

        ICommandQueue* Queue = nullptr;
        void* NativeWindowHandle = nullptr;
    };

    class IImage;
    class IImageView;

    class ISwapChain : public Memory::RefCountedObjectBase
    {
    public:
        ~ISwapChain() override = default;

        FE_CLASS_RTTI(ISwapChain, "B2D395D3-59B3-4552-9AC5-4B57BCB15259");

        virtual const SwapChainDesc& GetDesc() = 0;
        virtual void Present() = 0;
        virtual UInt32 GetCurrentImageIndex() = 0;
        virtual UInt32 GetCurrentFrameIndex() = 0;
        virtual UInt32 GetImageCount() = 0;
        virtual IImage* GetImage(UInt32 index) = 0;
        virtual IImage* GetCurrentImage() = 0;

        virtual eastl::vector<IImageView*> GetRTVs() = 0;
        virtual IImageView* GetDSV() = 0;
    };
} // namespace FE::Osmium
