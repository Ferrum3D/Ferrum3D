using System;
using Ferrum.Core.Utils;

namespace Ferrum.Osmium.AssetStreaming
{
    public interface IStreamedAsset : IDisposable
    {
        bool IsInCpuMemory { get; }
        bool IsInGpuMemory { get; }
        void SetAsset(in Uuid assetId);
        void LoadToCpuMemory();
        void LoadToGpuMemory(in AssetStreamingContext streamingContext);
        void Unload(in AssetStreamingContext streamingContext);
    }
}
