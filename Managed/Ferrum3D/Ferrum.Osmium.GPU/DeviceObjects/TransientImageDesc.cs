using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct TransientImageDesc
    {
        public readonly Image.Desc Descriptor;
        public readonly ulong ResourceID;

        public TransientImageDesc(Image.Desc descriptor, ulong resourceId)
        {
            Descriptor = descriptor;
            ResourceID = resourceId;
        }
    }
}
