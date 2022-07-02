using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ImageBounds
    {
        public readonly Offset First;
        public readonly Offset Second;

        public ImageBounds(Offset first, Offset second)
        {
            First = first;
            Second = second;
        }

        public ImageBounds(Offset second)
        {
            First = Offset.Zero;
            Second = second;
        }
    }
}
