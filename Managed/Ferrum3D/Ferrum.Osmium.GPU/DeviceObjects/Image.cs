using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Image : UnmanagedObject
    {
        public ImageView DefaultView =>
            defaultView ?? (defaultView = new ImageView(CreateViewNative(Handle, ImageAspectFlags.Color)));

        public ImageView DepthStencilView =>
            depthStencilView ?? (depthStencilView = new ImageView(CreateViewNative(Handle, ImageAspectFlags.Depth)));

        private ImageView defaultView;
        private ImageView depthStencilView;

        public Image(IntPtr handle) : base(handle)
        {
        }

        public void AllocateMemory(MemoryType memoryType)
        {
            AllocateMemoryNative(Handle, (int)memoryType);
        }

        [DllImport("OsGPUBindings", EntryPoint = "IImage_CreateView")]
        private static extern IntPtr CreateViewNative(IntPtr self, ImageAspectFlags aspectFlags);

        [DllImport("OsGPUBindings", EntryPoint = "IImage_AllocateMemory")]
        private static extern void AllocateMemoryNative(IntPtr self, int memoryType);

        [DllImport("OsGPUBindings", EntryPoint = "IImage_Destruct")]
        private static extern void DestructNative(IntPtr handle);

        protected override void ReleaseUnmanagedResources()
        {
            defaultView?.Dispose();
            depthStencilView?.Dispose();
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Desc
        {
            public readonly Size ImageSize;

            public readonly Format ImageFormat;
            public readonly ImageDim Dimension;

            public readonly ImageBindFlags BindFlags;

            public readonly uint MipLevelCount;
            public readonly uint SampleCount;
            public readonly ushort ArraySize;

            public Desc(Size imageSize, Format imageFormat, ImageDim dimension, ImageBindFlags bindFlags,
                uint mipLevelCount, uint sampleCount, ushort arraySize)
            {
                ImageSize = imageSize;
                ImageFormat = imageFormat;
                Dimension = dimension;
                BindFlags = bindFlags;
                MipLevelCount = mipLevelCount;
                SampleCount = sampleCount;
                ArraySize = arraySize;
            }

            public static Desc Img1D(ImageBindFlags bindFlags, int width, Format format)
            {
                return Img1DArray(bindFlags, width, 1, format);
            }

            public static Desc Img1DArray(ImageBindFlags bindFlags, int width, ushort arraySize, Format format)
            {
                return new Desc(new Size(width), format, ImageDim.Image1D, bindFlags, 1, 1, arraySize);
            }

            public static Desc Img2D(ImageBindFlags bindFlags, int width, int height, Format format)
            {
                return Img2DArray(bindFlags, width, height, 1, format);
            }

            public static Desc Img2DArray(ImageBindFlags bindFlags, int width, int height, ushort arraySize,
                Format format)
            {
                return new Desc(new Size(width, height), format, ImageDim.Image2D, bindFlags, 1, 1, arraySize);
            }

            public static Desc ImgCubemap(ImageBindFlags bindFlags, int width, Format format)
            {
                return ImgCubemapArray(bindFlags, width, 1, format);
            }

            public static Desc ImgCubemapArray(ImageBindFlags bindFlags, int width, ushort arraySize, Format format)
            {
                arraySize *= 6;
                return new Desc(new Size(width, width), format, ImageDim.ImageCubemap, bindFlags, 1, 1, arraySize);
            }

            public static Desc Img3D(ImageBindFlags bindFlags, int width, int height, int depth, Format format)
            {
                return new Desc(new Size(width, height, depth), format, ImageDim.Image3D, bindFlags, 1, 1, 1);
            }
        }
    }
}
