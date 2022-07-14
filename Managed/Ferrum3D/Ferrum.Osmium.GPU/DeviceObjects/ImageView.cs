using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class ImageView : UnmanagedObject
    {
        public ImageSubresourceRange SubresourceRange => Descriptor.SubresourceRange;
        public Format Format => Descriptor.Format;
        public ImageDim Dimension => Descriptor.Dimension;

        public Desc Descriptor { get; }

        public ImageView(IntPtr handle) : base(handle)
        {
            GetDescNative(handle, out var desc);
            Descriptor = new Desc(desc);
        }

        [DllImport("OsGPUBindings", EntryPoint = "IImageView_GetDesc")]
        private static extern void GetDescNative(IntPtr self, out DescNative desc);

        [DllImport("OsGPUBindings", EntryPoint = "IImageView_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct DescNative
        {
            public readonly ImageSubresourceRange SubresourceRange;
            public readonly Format Format;
            public readonly IntPtr Image;
            public readonly ImageDim Dimension;

            internal DescNative(Desc nativeDesc)
            {
                SubresourceRange = nativeDesc.SubresourceRange;
                Format = nativeDesc.Format;
                Image = nativeDesc.Image.Handle;
                Dimension = nativeDesc.Dimension;
            }
        }

        public struct Desc
        {
            public readonly ImageSubresourceRange SubresourceRange;
            public readonly Format Format;
            public readonly Image Image;
            public readonly ImageDim Dimension;

            internal Desc(DescNative descNative)
            {
                SubresourceRange = descNative.SubresourceRange;
                Format = descNative.Format;
                Image = new Image(descNative.Image);
                Image.IsOwned = false;
                Dimension = descNative.Dimension;
            }

            public Desc(ImageSubresourceRange subresourceRange, Format format, Image image, ImageDim dimension)
            {
                SubresourceRange = subresourceRange;
                Format = format;
                Image = image;
                Dimension = dimension;
            }
        }
    }
}
