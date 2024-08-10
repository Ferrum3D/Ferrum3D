#pragma once
#include <OsGPU/Image/ImageFormat.h>

namespace FE::Osmium
{
    class ICommandQueue;

    struct SwapChainDesc
    {
        FE_RTTI_Base(SwapChainDesc, "19401C0C-A89C-4393-8D40-F669AB8B128C");

        uint32_t ImageCount = 3;
        uint32_t FrameCount = 2;
        uint32_t ImageWidth = 0;
        uint32_t ImageHeight = 0;
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

        FE_RTTI_Class(ISwapChain, "B2D395D3-59B3-4552-9AC5-4B57BCB15259");

        virtual const SwapChainDesc& GetDesc() = 0;
        virtual void Present() = 0;
        virtual uint32_t GetCurrentImageIndex() = 0;
        virtual uint32_t GetCurrentFrameIndex() = 0;
        virtual uint32_t GetImageCount() = 0;
        virtual IImage* GetImage(uint32_t index) = 0;
        virtual IImage* GetCurrentImage() = 0;

        virtual eastl::vector<IImageView*> GetRTVs() = 0;
        virtual IImageView* GetDSV() = 0;
    };
} // namespace FE::Osmium
