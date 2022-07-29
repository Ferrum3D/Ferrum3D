using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Utils;
using Ferrum.Osmium.Assets;
using Ferrum.Osmium.FrameGraph.CommandLists;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.VertexInput;

namespace Ferrum.Osmium.AssetStreaming
{
    [Flags]
    public enum MeshFlags
    {
        None,
        CpuAccessible
    }

    public sealed class Mesh : IStreamedAsset
    {
        public bool IsInCpuMemory { get; private set; }
        public bool IsInGpuMemory { get; private set; }

        public AssetRef<MeshAsset> Asset { get; private set; }
        public MeshFlags Flags { get; set; }
        public bool IsReadable => (Flags & MeshFlags.CpuAccessible) != 0;

        public InputStreamLayout VertexLayout { get; }

        public VertexBufferSlice Vertices { get; private set; }
        public IndexBufferSlice Indices { get; private set; }

        public Mesh()
        {
            VertexLayout = MeshAsset.InputStreamLayout;
        }

        public void SetAsset(in Uuid assetId)
        {
            Asset = new AssetRef<MeshAsset>(in assetId);
        }

        public void LoadToCpuMemory()
        {
            Asset.LoadSync();
            IsInCpuMemory = true;
        }

        public void LoadToGpuMemory(in AssetStreamingContext streamingContext)
        {
            using var stagingVertex = Asset.Storage.CreateVertexStagingBuffer(streamingContext.Device);
            using var stagingIndex = Asset.Storage.CreateIndexStagingBuffer(streamingContext.Device);

            var vertexBuffer = streamingContext.Device.CreateBuffer(BindFlags.VertexBuffer, Asset.Storage.VertexSize);
            vertexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            var indexBuffer = streamingContext.Device.CreateBuffer(BindFlags.IndexBuffer, Asset.Storage.IndexSize);
            indexBuffer.AllocateMemory(MemoryType.DeviceLocal);

            if (!IsReadable)
            {
                Asset.Reset();
            }

            streamingContext.CommandBuffer.CopyBuffers(stagingVertex, vertexBuffer, Asset.Storage.VertexSize);
            streamingContext.CommandBuffer.CopyBuffers(stagingIndex, indexBuffer, Asset.Storage.IndexSize);

            Vertices = new VertexBufferSlice(vertexBuffer, 0, Asset.Storage.VertexSize, VertexLayout.ByteStride);
            Indices = new IndexBufferSlice(indexBuffer, 0, Asset.Storage.IndexSize);

            IsInGpuMemory = true;
        }

        public void Unload(in AssetStreamingContext streamingContext)
        {
            Asset.Reset();
            Vertices.Buffer?.Dispose();
            Indices.Buffer?.Dispose();
            Vertices = VertexBufferSlice.Empty;
            Indices = IndexBufferSlice.Empty;
        }

        public void Dispose()
        {
            Asset.Dispose();
            Vertices.Buffer?.Dispose();
            Indices.Buffer?.Dispose();
        }
    }
}
