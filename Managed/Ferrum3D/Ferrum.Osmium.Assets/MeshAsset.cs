using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.VertexInput;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.Assets
{
    public readonly struct MeshAsset : IAssetStorage<MeshAsset>
    {
        public uint IndexCount => (uint)IndexSize / 4;
        public ulong VertexSize { get; }
        public ulong IndexSize { get; }
        public IntPtr VertexData { get; }
        public IntPtr IndexData { get; }

        public IntPtr Handle { get; }

        public static readonly InputStreamLayout InputStreamLayout = new InputStreamLayout.Builder()
            .AddBuffer(InputStreamRate.PerVertex)
            .AddAttribute(Format.R32G32B32_SFloat, "POSITION")
            .AddAttribute(Format.R32G32_SFloat, "TEXCOORD")
            .Build()
            .Build();

        private MeshAsset(IntPtr handle, ulong vertexSize, ulong indexSize, IntPtr vertexData, IntPtr indexData)
        {
            Handle = handle;
            VertexSize = vertexSize;
            IndexSize = indexSize;
            VertexData = vertexData;
            IndexData = indexData;
        }

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

        public MeshAsset WithNativePointer(IntPtr pointer)
        {
            return new MeshAsset(pointer,
                VertexSizeNative(pointer),
                IndexSizeNative(pointer),
                VertexDataNative(pointer),
                IndexDataNative(pointer));
        }

        public MeshAsset Reset()
        {
            return new MeshAsset();
        }

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_VertexSize")]
        private static extern ulong VertexSizeNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_IndexSize")]
        private static extern ulong IndexSizeNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_VertexData")]
        private static extern IntPtr VertexDataNative(IntPtr self);

        [DllImport("OsAssetsBindings", EntryPoint = "MeshAssetStorage_IndexData")]
        private static extern IntPtr IndexDataNative(IntPtr self);
    }
}
