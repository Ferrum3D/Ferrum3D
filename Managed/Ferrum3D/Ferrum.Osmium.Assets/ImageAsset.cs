using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Math;
using Ferrum.Core.Utils;
using Ferrum.Osmium.GPU.DeviceObjects;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.Assets
{
    public sealed class ImageAsset : Asset
    {
        public Color this[int row, int column] => Color.FromUInt32(PixelValueAt(row, column));

        public int Width => (int)ImageSize.Width;
        public int Height => (int)ImageSize.Height;
        public ulong ByteSize { get; private set; }

        public IntPtr DataHandle { get; private set; }
        public Size ImageSize { get; private set; }

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

        protected override void Initialize()
        {
            ImageSize = new Size(WidthNative(Handle), HeightNative(Handle));
            ByteSize = SizeNative(Handle);
            DataHandle = DataNative(Handle);
        }

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Load")]
        private static extern IntPtr LoadNative(IntPtr manager, in Uuid assetId);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Data")]
        private static extern IntPtr DataNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Size")]
        private static extern ulong SizeNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Width")]
        private static extern int WidthNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Height")]
        private static extern int HeightNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "ImageAssetStorage_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        protected override IntPtr LoadByIdImpl(IntPtr manager, in Uuid assetId)
        {
            return LoadNative(manager, in assetId);
        }
    }
}
