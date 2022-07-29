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
        None = 0,
        CpuAccessible = 1 << 0
    }

    public sealed class Mesh : IStreamedAsset
    {
        public bool IsInCpuMemory { get; private set; }
        public bool IsInGpuMemory { get; private set; }

        public MeshFlags Flags { get; set; }
        public bool IsReadable => (Flags & MeshFlags.CpuAccessible) != 0;

        public InputStreamLayout VertexLayout { get; }

        public VertexBufferSlice Vertices { get; private set; }
        public IndexBufferSlice Indices { get; private set; }

        private AssetRef<MeshAsset> asset;

        public Mesh()
        {
            VertexLayout = MeshAsset.InputStreamLayout;
        }

        public void SetAsset(in Uuid assetId)
        {
            asset = new AssetRef<MeshAsset>(in assetId);
        }

        public void LoadToCpuMemory()
        {
            asset = asset.LoadSync();
            IsInCpuMemory = true;
        }

        public void LoadToGpuMemory(in AssetStreamingContext streamingContext)
        {
            using var stagingVertex = asset.Storage.CreateVertexStagingBuffer(streamingContext.Device);
            using var stagingIndex = asset.Storage.CreateIndexStagingBuffer(streamingContext.Device);

            var vertexSize = asset.Storage.VertexSize;
            var indexSize = asset.Storage.IndexSize;

            var vertexBuffer = streamingContext.Device.CreateBuffer(BindFlags.VertexBuffer, vertexSize);
            vertexBuffer.AllocateMemory(MemoryType.DeviceLocal);
            var indexBuffer = streamingContext.Device.CreateBuffer(BindFlags.IndexBuffer, indexSize);
            indexBuffer.AllocateMemory(MemoryType.DeviceLocal);

            if (!IsReadable)
            {
                asset = asset.Reset();
            }

            streamingContext.CommandBuffer.CopyBuffers(stagingVertex, vertexBuffer, vertexSize);
            streamingContext.CommandBuffer.CopyBuffers(stagingIndex, indexBuffer, indexSize);

            Vertices = new VertexBufferSlice(vertexBuffer, 0, vertexSize, VertexLayout.ByteStride);
            Indices = new IndexBufferSlice(indexBuffer, 0, indexSize);

            IsInGpuMemory = true;
        }

        public void Unload(in AssetStreamingContext streamingContext)
        {
            asset = asset.Reset();
            Vertices.Buffer?.Dispose();
            Indices.Buffer?.Dispose();
            Vertices = VertexBufferSlice.Empty;
            Indices = IndexBufferSlice.Empty;
        }

        public void Dispose()
        {
            asset.Dispose();
            Vertices.Buffer?.Dispose();
            Indices.Buffer?.Dispose();
        }
    }
}
