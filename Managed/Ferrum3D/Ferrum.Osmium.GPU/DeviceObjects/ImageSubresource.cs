using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageSubresource
    {
        public static ImageSubresource Default = new ImageSubresource(0, 0, ImageAspect.Color);
        public readonly ushort MipSlice;
        public readonly ushort ArraySlice;

        public readonly ImageAspect Aspect;

        public ImageSubresource(int mipSlice, int arraySlice = 0, ImageAspect aspect = ImageAspect.Color)
        {
            MipSlice = (ushort)mipSlice;
            ArraySlice = (ushort)arraySlice;
            Aspect = aspect;
        }
    }
}
