using System;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [Flags]
    public enum ImageAspectFlags
    {
        None,
        RenderTarget = 1 << ImageAspect.RenderTarget,
        Depth = 1 << ImageAspect.Depth,
        Stencil = 1 << ImageAspect.Stencil,
        DepthStencil = Depth | Stencil,
        All = Depth | Stencil | RenderTarget
    }
}
