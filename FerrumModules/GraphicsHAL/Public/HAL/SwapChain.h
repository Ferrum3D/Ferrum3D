#pragma once
#include <HAL/Common/BaseTypes.h>
#include <HAL/ImageFormat.h>

namespace FE::Graphics::HAL
{
    class CommandQueue;

    struct SwapchainDesc
    {
        FE_RTTI_Base(SwapchainDesc, "19401C0C-A89C-4393-8D40-F669AB8B128C");

        uint32_t ImageCount = 3;
        uint32_t FrameCount = 2;
        uint32_t ImageWidth = 0;
        uint32_t ImageHeight = 0;
        bool VerticalSync = false;
        Format Format = Format::None;

        CommandQueue* Queue = nullptr;
        void* NativeWindowHandle = nullptr;
    };


    class Image;
    class ImageView;

    class Swapchain : public DeviceObject
    {
    public:
        ~Swapchain() override = default;

        FE_RTTI_Class(Swapchain, "B2D395D3-59B3-4552-9AC5-4B57BCB15259");

        virtual ResultCode Init(const SwapchainDesc& desc) = 0;

        virtual const SwapchainDesc& GetDesc() = 0;
        virtual void Present() = 0;
        virtual uint32_t GetCurrentImageIndex() = 0;
        virtual uint32_t GetCurrentFrameIndex() = 0;
        virtual uint32_t GetImageCount() = 0;
        virtual Image* GetImage(uint32_t index) = 0;
        virtual Image* GetCurrentImage() = 0;

        virtual festd::span<ImageView*> GetRTVs() = 0;
        virtual ImageView* GetDSV() = 0;
    };
} // namespace FE::Graphics::HAL
