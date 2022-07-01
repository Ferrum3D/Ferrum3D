using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Assets
{
    public class AssetManager : IDisposable
    {
        public static IntPtr Handle { get; private set; }

        public AssetManager(string assetIndexFilename)
        {
            Handle = CreateDevelopmentModeNative(assetIndexFilename);
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        [DllImport("FeCoreBindings", EntryPoint = "IAssetManager_CreateDevelopmentMode")]
        private static extern IntPtr CreateDevelopmentModeNative(string assetIndexFilename);

        [DllImport("FeCoreBindings", EntryPoint = "IAssetManager_Destruct")]
        private static extern void DestructNative(IntPtr self);

        private static void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
            Handle = IntPtr.Zero;
        }

        ~AssetManager()
        {
            ReleaseUnmanagedResources();
        }
    }
}
