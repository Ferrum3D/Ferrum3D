#pragma once
#include <FeCore/Math/Aabb.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Graphics::RHI
{
    struct WindowDesc final
    {
        uint32_t m_width = 0;
        uint32_t m_height = 0;

        festd::string_view m_title;
    };


    struct IWindow : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IWindow, "2E09CD62-42A4-4E0D-BC2C-B11E849FBEAF");

        virtual ResultCode Init(const WindowDesc& desc) = 0;

        virtual void PollEvents() = 0;
        virtual bool CloseRequested() = 0;
        virtual void* GetNativeHandle() = 0;
        virtual RectF CreateViewport() = 0;
        virtual RectInt CreateScissor() = 0;
    };
} // namespace FE::Graphics::RHI
