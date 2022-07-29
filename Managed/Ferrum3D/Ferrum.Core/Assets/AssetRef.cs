using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Assets
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct AssetRef<T> : IDisposable
        where T : unmanaged, IAssetStorage<T>
    {
        public readonly T Storage;
        public Uuid AssetId => asset.AssetId;
        private readonly Asset asset;

        public AssetRef(in Asset asset)
        {
            this.asset = asset;
            Storage = new T();
        }

        public AssetRef(in Uuid id)
        {
            asset.AssetId = id;
            asset.Storage = IntPtr.Zero;
            Storage = new T();
        }

        private AssetRef(in Asset asset, in T storage)
        {
            this.asset = asset;
            Storage = storage;
        }

        public AssetRef<T> Copy()
        {
            asset.AddStrongRef();
            return new AssetRef<T>(asset);
        }

        public AssetRef<T> LoadSync()
        {
            var a = asset.LoadSync();
            var s = Storage;
            return new AssetRef<T>(a, s.WithNativePointer(a.Storage));
        }

        public AssetRef<T> Reset()
        {
            Dispose();
            return new AssetRef<T>();
        }

        public void Dispose()
        {
            asset.ReleaseStrongRef();
        }
    }
}
