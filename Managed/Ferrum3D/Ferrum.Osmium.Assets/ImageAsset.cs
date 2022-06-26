using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Math;

namespace Ferrum.Osmium.Assets
{
    public sealed class ImageAsset : Asset
    {
        public Color this[int row, int column] => Color.FromUInt32(PixelValueAt(row, column));

        public int Width { get; private set; }
        public int Height { get; private set; }
        public ulong ByteSize { get; private set; }

        public ImageAsset() : base(IntPtr.Zero)
        {
        }

        public uint PixelValueAt(int row, int column)
        {
            unsafe
            {
                var ptr = (uint*)DataNative(Handle);
                return ptr![row * Width + column];
            }
        }

        protected override void Initialize()
        {
            Width = WidthNative(Handle);
            Height = HeightNative(Handle);
            ByteSize = SizeNative(Handle);
        }

        [DllImport("OsmiumAssetsBindings", EntryPoint = "ImageAssetStorage_Load")]
        private static extern IntPtr LoadNative(IntPtr manager, string assetId);

        [DllImport("OsmiumAssetsBindings", EntryPoint = "ImageAssetStorage_Data")]
        private static extern IntPtr DataNative(IntPtr self);

        [DllImport("OsmiumAssetsBindings", EntryPoint = "ImageAssetStorage_Size")]
        private static extern ulong SizeNative(IntPtr self);

        [DllImport("OsmiumAssetsBindings", EntryPoint = "ImageAssetStorage_Width")]
        private static extern int WidthNative(IntPtr self);

        [DllImport("OsmiumAssetsBindings", EntryPoint = "ImageAssetStorage_Height")]
        private static extern int HeightNative(IntPtr self);

        [DllImport("OsmiumAssetsBindings", EntryPoint = "ImageAssetStorage_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        protected override IntPtr LoadByIdImpl(IntPtr manager, Guid assetId)
        {
            return LoadNative(manager, assetId.ToString());
        }
    }
}
