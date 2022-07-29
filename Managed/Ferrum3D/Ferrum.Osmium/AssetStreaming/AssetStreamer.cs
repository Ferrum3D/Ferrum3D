using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Containers;
using Ferrum.Core.Utils;
using Ferrum.Osmium.Assets;

namespace Ferrum.Osmium.AssetStreaming
{
    public sealed class AssetStreamer : IDisposable
    {
        private readonly DisposableList<IStreamedAsset> assets = new() { null };
        private readonly List<Handle> pendingLoadToCpuMemory = new();
        private readonly List<Handle> pendingLoadToGpuMemory = new();
        private readonly List<Handle> pendingUnload = new();

        public Handle AddAsset<T>(in Uuid assetId)
            where T : IStreamedAsset, new()
        {
            assets.Add(new T());
            var asset = assets.Last();
            asset.SetAsset(in assetId);
            return new Handle(assets.Count - 1);
        }

        public void QueueLoad(Handle handle)
        {
            pendingLoadToCpuMemory.Add(handle);
        }

        public void QueueUnload(Handle handle)
        {
            pendingUnload.Add(handle);
        }

        public T GetAsset<T>(Handle handle)
            where T : class, IStreamedAsset
        {
            return assets[handle.AssetIndex] as T;
        }

        public void Update(in AssetStreamingContext streamingContext)
        {
            foreach (var handle in pendingUnload)
            {
                assets[handle.AssetIndex].Unload(in streamingContext);
                pendingLoadToCpuMemory.Remove(handle);
                pendingLoadToGpuMemory.Remove(handle);
            }

            foreach (var asset in pendingLoadToCpuMemory)
            {
                assets[asset.AssetIndex].LoadToCpuMemory();
            }

            foreach (var asset in pendingLoadToGpuMemory)
            {
                assets[asset.AssetIndex].LoadToGpuMemory(in streamingContext);
            }

            pendingLoadToGpuMemory.Clear();
            pendingLoadToGpuMemory.AddRange(pendingLoadToCpuMemory);
            pendingLoadToCpuMemory.Clear();
        }

        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Handle
        {
            internal readonly int AssetIndex;

            internal Handle(int assetIndex)
            {
                AssetIndex = assetIndex;
            }

            public bool Equals(Handle other)
            {
                return AssetIndex == other.AssetIndex;
            }

            public override bool Equals(object obj)
            {
                return obj is Handle other && Equals(other);
            }

            public override int GetHashCode()
            {
                return AssetIndex;
            }

            public static bool operator ==(Handle left, Handle right)
            {
                return left.Equals(right);
            }

            public static bool operator !=(Handle left, Handle right)
            {
                return !left.Equals(right);
            }

            public static implicit operator bool(in Handle handle)
            {
                return !handle.IsNull();
            }

            public bool IsNull()
            {
                return AssetIndex == 0;
            }
        }

        public void Dispose()
        {
            assets.Dispose();
        }
    }
}
