#pragma once
#include <HAL/Common/BaseTypes.h>
#include <HAL/ImageFormat.h>

namespace FE::Graphics::HAL
{
    struct CommandQueue;

    struct SwapchainDesc final
    {
        uint32_t FrameCount = 3;
        uint32_t ImageWidth = 0;
        uint32_t ImageHeight = 0;
        bool VerticalSync = false;
        Format Format = Format::kUndefined;

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

        virtual const SwapchainDesc& GetDesc() const = 0;

        virtual void BeginFrame(const FenceSyncPoint& signalFence) = 0;
        virtual void Present(const FenceSyncPoint& waitFence) = 0;

        virtual uint32_t GetCurrentImageIndex() const = 0;
        virtual uint32_t GetImageCount() const = 0;

        virtual festd::span<ImageView* const> GetRTVs() const = 0;
        virtual ImageView* GetDSV() const = 0;
    };
} // namespace FE::Graphics::HAL
