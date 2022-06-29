using System;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [Flags]
    public enum ImageAspectFlags
    {
        None,
        Color = 1 << ImageAspect.Color,
        Depth = 1 << ImageAspect.Depth,
        Stencil = 1 << ImageAspect.Stencil,
        DepthStencil = Depth | Stencil,
        All = Depth | Stencil | Color
    }
}
