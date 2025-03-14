#pragma once
#include <FeCore/Math/Rect.h>

namespace FE::Framework::Core
{
    struct NativeMonitorHandle final : public TypedHandle<NativeMonitorHandle, uint64_t>
    {
    };


    struct PlatformMonitorInfo final
    {
        RectInt m_rect;                     //!< The area displayed on the monitor.
        RectInt m_workRect;                 //!< Rect without task bars / sidebars / menu bars.
        float m_dpiScale;                   //!< 1.0f = 96 DPI.
        NativeMonitorHandle m_nativeHandle; //!< Native monitor handle.
    };
} // namespace FE::Framework::Core
