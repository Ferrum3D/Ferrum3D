using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Math;
using Ferrum.Osmium.GPU.DeviceObjects;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.Assets
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageAsset : IAssetStorage<ImageAsset>
    {
        public Color this[int row, int column] => Color.FromUInt32(PixelValueAt(row, column));

        public int Width => (int)ImageSize.Width;
        public int Height => (int)ImageSize.Height;
        public ulong ByteSize { get; }

        public IntPtr DataHandle { get; }
        public Size ImageSize { get; }

        public IntPtr Handle { get; }

        private ImageAsset(IntPtr handle, Size imageSize, ulong byteSize, IntPtr dataHandle)
        {
            Handle = handle;
            ImageSize = imageSize;
            ByteSize = byteSize;
            DataHandle = dataHandle;
        }

        public uint PixelValueAt(int row, int column)
        {
            unsafe
            {
                var ptr = (uint*)DataHandle;
                return ptr![row * Width + column];
            }
        }

        public Buffer CreateStagingBuffer(Device device)
        {
            var stagingBuffer = device.CreateBuffer(BindFlags.None, ByteSize);
            stagingBuffer.AllocateMemory(MemoryType.HostVisible);
            stagingBuffer.UpdateData(DataHandle);
            return stagingBuffer;
        }

        public ImageAsset WithNativePointer(IntPtr pointer)
        {
            return new ImageAsset(pointer,
                new Size(WidthNative(pointer), HeightNative(pointer)),
                SizeNative(pointer),
                DataNative(pointer));
        }

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Data")]
        private static extern IntPtr DataNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Size")]
        private static extern ulong SizeNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Width")]
        private static extern int WidthNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Height")]
        private static extern int HeightNative(IntPtr self);
    }
}
