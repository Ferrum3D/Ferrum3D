using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Osmium.GPU.DeviceObjects;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.Assets
{
    public sealed class MeshAsset : Asset
    {
        public uint IndexCount => (uint)IndexSize / 4;
        public ulong VertexSize { get; private set; }
        public ulong IndexSize { get; private set; }
        public IntPtr VertexData { get; private set; }
        public IntPtr IndexData { get; private set; }

        public Buffer CreateVertexStagingBuffer(Device device)
        {
            var stagingBuffer = device.CreateBuffer(BindFlags.None, VertexSize);
            stagingBuffer.AllocateMemory(MemoryType.HostVisible);
            stagingBuffer.UpdateData(VertexData);
            return stagingBuffer;
        }

        public Buffer CreateIndexStagingBuffer(Device device)
        {
            var stagingBuffer = device.CreateBuffer(BindFlags.None, IndexSize);
            stagingBuffer.AllocateMemory(MemoryType.HostVisible);
            stagingBuffer.UpdateData(IndexData);
            return stagingBuffer;
        }

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_Load")]
        private static extern IntPtr LoadNative(IntPtr manager, string assetId);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_VertexSize")]
        private static extern ulong VertexSizeNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_IndexSize")]
        private static extern ulong IndexSizeNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_VertexData")]
        private static extern IntPtr VertexDataNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_IndexData")]
        private static extern IntPtr IndexDataNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void Initialize()
        {
            VertexSize = VertexSizeNative(Handle);
            IndexSize = IndexSizeNative(Handle);
            VertexData = VertexDataNative(Handle);
            IndexData = IndexDataNative(Handle);
        }

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
