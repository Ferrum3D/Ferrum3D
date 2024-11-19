#pragma once
#include <Graphics/RHI/Common/BaseTypes.h>
#include <Graphics/RHI/ImageFormat.h>

namespace FE::Graphics::RHI
{
    struct CommandQueue;

    struct SwapchainDesc final
    {
        uint32_t m_frameCount = 2;
        uint32_t m_imageWidth = 0;
        uint32_t m_imageHeight = 0;
        bool m_verticalSync = false;
        Format m_format = Format::kUndefined;

        CommandQueue* m_queue = nullptr;
        void* m_nativeWindowHandle = nullptr;
    };


    struct Image;
    struct ImageView;

    struct Swapchain : public DeviceObject
    {
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
} // namespace FE::Graphics::RHI
