using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Framework;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU
{
    public class OsmiumGpuModule : NativeModuleFramework
    {
        public override DynamicLibrary Library => Factory.Library;
        private const string LibraryPath = "OsGPU";

        private OsmiumGpuModule(IntPtr handle) : base(LibraryPath, handle)
        {
        }

        public void Initialize(Desc desc)
        {
            base.Initialize();
            InitializeNative(Handle, ref desc);
        }

        public Instance CreateInstance()
        {
            return new Instance(CreateInstanceNative(Handle));
        }

        [DllImport(LibraryPath + "Bindings", EntryPoint = "OsmiumGPUModule_Initialize")]
        private static extern void InitializeNative(IntPtr self, ref Desc desc);

        [DllImport(LibraryPath + "Bindings", EntryPoint = "OsmiumGPUModule_CreateInstance")]
        private static extern IntPtr CreateInstanceNative(IntPtr self);

        public sealed class Factory : NativeModuleFrameworkFactory<OsmiumGpuModule>
        {
            public Factory() : base(LibraryPath)
            {
            }

            protected override OsmiumGpuModule CreateFramework(IntPtr handle)
            {
                return new OsmiumGpuModule(handle);
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public readonly struct Desc
        {
            public readonly string ApplicationName;
            public readonly GraphicsApi Api;

            public Desc(string applicationName, GraphicsApi api)
            {
                ApplicationName = applicationName;
                Api = api;
            }
        }
    }
}
