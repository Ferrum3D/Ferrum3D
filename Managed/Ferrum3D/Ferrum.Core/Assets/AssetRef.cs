using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Assets
{
    [StructLayout(LayoutKind.Sequential)]
    public struct AssetRef<T> : IDisposable
        where T : unmanaged, IAssetStorage<T>
    {
        public T Storage { get; private set; }
        private readonly Asset asset;

        public AssetRef(in Asset asset)
        {
            asset.AddStrongRef();
            this.asset = asset;
            Storage = new T();
        }

        public AssetRef(in Uuid id)
        {
            asset.AssetId = id;
            asset.Storage = IntPtr.Zero;
            Storage = new T();
        }

        public readonly AssetRef<T> Copy()
        {
            return new AssetRef<T>(asset);
        }

        public void LoadSync()
        {
            asset.LoadSync();
            Storage = Storage.WithNativePointer(asset.Storage);
        }

        public void Dispose()
        {
            asset.ReleaseStrongRef();
        }
    }
}
