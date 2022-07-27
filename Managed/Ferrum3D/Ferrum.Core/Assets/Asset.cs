using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Assets
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Asset
    {
        public Uuid AssetId;
        public IntPtr Storage;

        public readonly void AddStrongRef()
        {
            AddStrongRefNative(in this);
        }

        public readonly void ReleaseStrongRef()
        {
            ReleaseStrongRefNative(in this);
        }

        public readonly void LoadSync()
        {
            LoadSyncNative(in this);
        }

        [DllImport("FeCoreBindings", EntryPoint = "Asset_AddStrongRef")]
        private static extern void AddStrongRefNative(in Asset self);

        [DllImport("FeCoreBindings", EntryPoint = "Asset_ReleaseStrongRef")]
        private static extern void ReleaseStrongRefNative(in Asset self);

        [DllImport("FeCoreBindings", EntryPoint = "Asset_LoadSync")]
        private static extern void LoadSyncNative(in Asset self);
    }
}
