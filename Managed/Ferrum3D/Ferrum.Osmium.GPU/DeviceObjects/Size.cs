using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct Size
    {
        public readonly ulong Width;
        public readonly ulong Height;
        public readonly ulong Depth;

        public Size(int width, int height = 1, int depth = 1)
        {
            Width = (ulong)width;
            Height = (ulong)height;
            Depth = (ulong)depth;
        }
        
        public Size(ulong width, ulong height = 1, ulong depth = 1)
        {
            Width = width;
            Height = height;
            Depth = depth;
        }
    }
}
