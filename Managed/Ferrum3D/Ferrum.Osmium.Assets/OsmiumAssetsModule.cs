using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Ferrum.Core.Assets;
using Ferrum.Core.Framework;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.Assets
{
    public class OsmiumAssetsModule : NativeModuleFramework
    {
        public override DynamicLibrary Library => Factory.Library;
        private const string LibraryPath = "OsAssets";

        private OsmiumAssetsModule(IntPtr handle) : base(LibraryPath, handle)
        {
        }

        public void Initialize(Desc desc)
        {
            base.Initialize();
            Debug.Assert(AssetManager.Handle != IntPtr.Zero);
            InitializeNative(Handle, ref desc);
        }

        [DllImport(LibraryPath + "Bindings", EntryPoint = "OsmiumAssetsModule_Initialize")]
        private static extern void InitializeNative(IntPtr self, ref Desc desc);

        public sealed class Factory : NativeModuleFrameworkFactory<OsmiumAssetsModule>
        {
            public Factory() : base(LibraryPath)
            {
            }

            protected override FrameworkBase CreateFramework(IntPtr handle)
            {
                return new OsmiumAssetsModule(handle);
            }
        }

        public readonly struct Desc
        {
        }
    }
}
