using System;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [Flags]
    public enum ImageBindFlags
    {
        None = 0,

        ShaderRead = 1 << 0,
        UnorderedAccess = 1 << 1,

        Color = 1 << 2,
        Depth = 1 << 3,
        Stencil = 1 << 4,

        TransferRead = 1 << 5,
        TransferWrite = 1 << 6,

        DepthStencil = Depth | Stencil,
        TransferReadWrite = TransferRead | TransferWrite
    }
}
