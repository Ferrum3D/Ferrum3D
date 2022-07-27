using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Console;
using Ferrum.Core.Containers;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Assets
{
    public interface IAssetStorage<out T>
    {
        T WithNativePointer(IntPtr pointer);
    }

    internal static class AssetBindings
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct NativeData
        {
            public Uuid AssetId;
            public IntPtr Storage;
        }

        [DllImport("FeCoreBindings", EntryPoint = "Asset_AddStrongRef")]
        public static extern void AddStrongRefNative(ref NativeData self);

        [DllImport("FeCoreBindings", EntryPoint = "Asset_ReleaseStrongRef")]
        public static extern void ReleaseStrongRefNative(ref NativeData self);

        [DllImport("FeCoreBindings", EntryPoint = "Asset_LoadSync")]
        public static extern void LoadSyncNative(ref NativeData self);
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct AssetRef<T> : IDisposable
        where T : unmanaged, IAssetStorage<T>
    {
        public T Storage { get; private set; }
        private AssetBindings.NativeData nativeData;

        public AssetRef(in Uuid id)
        {
            nativeData.AssetId = id;
            nativeData.Storage = IntPtr.Zero;
            Storage = new T();
        }

        public AssetRef<T> Copy()
        {
            AssetBindings.AddStrongRefNative(ref nativeData);
            return new AssetRef<T>
            {
                nativeData = nativeData
            };
        }

        public void LoadSync()
        {
            AssetBindings.LoadSyncNative(ref nativeData);
            Storage = Storage.WithNativePointer(nativeData.Storage);
        }

        public void Dispose()
        {
            AssetBindings.ReleaseStrongRefNative(ref nativeData);
        }
    }
}
