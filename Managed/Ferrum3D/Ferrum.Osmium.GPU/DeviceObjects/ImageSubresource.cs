using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageSubresource
    {
        public readonly ushort MipSlice;
        public readonly ushort ArraySlice;

        public readonly ImageAspect Aspect;

        public static ImageSubresource Default = new ImageSubresource(0, 0, ImageAspect.Color);

        public ImageSubresource(ushort mipSlice, ushort arraySlice, ImageAspect aspect)
        {
            MipSlice = mipSlice;
            ArraySlice = arraySlice;
            Aspect = aspect;
        }
    }
}
