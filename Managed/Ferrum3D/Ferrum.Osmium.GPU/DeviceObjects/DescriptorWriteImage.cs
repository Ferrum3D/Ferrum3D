using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public readonly struct DescriptorWriteImage
    {
        public readonly ImageView ImageView;
        public readonly int Binding;
        public readonly int ArrayIndex;

        public DescriptorWriteImage(ImageView imageView, int binding = 0, int arrayIndex = 0)
        {
            ImageView = imageView;
            Binding = binding;
            ArrayIndex = arrayIndex;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly IntPtr Image;
            public readonly uint Binding;
            public readonly uint ArrayIndex;

            public Native(DescriptorWriteImage write)
            {
                Image = write.ImageView?.Handle ?? IntPtr.Zero;
                Binding = (uint)write.Binding;
                ArrayIndex = (uint)write.ArrayIndex;
            }
        }
    }
}
