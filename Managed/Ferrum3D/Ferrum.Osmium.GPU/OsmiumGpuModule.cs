using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Framework;
using Ferrum.Osmium.GPU.DeviceObjects;
using Factory = Ferrum.Core.Framework.NativeModuleFrameworkFactory<Ferrum.Osmium.GPU.OsmiumGpuModule>;

namespace Ferrum.Osmium.GPU
{
    public class OsmiumGpuModule : NativeModuleFramework
    {
        private const string LibraryName = "OsGPU";

        public static IFrameworkFactory CreateFactory()
        {
            return new Factory(LibraryName);
        }

        public void Initialize(Desc desc)
        {
            var init = Factory.Library.GetFunction<InitializeNative>("OsmiumGpuModule_Initialize");
            init(Handle, ref desc);
        }

        public Instance CreateInstance()
        {
            var createInstance = Factory.Library.GetFunction<CreateInstanceNative>("OsmiumGpuModule_CreateInstance");
            return new Instance(createInstance(Handle));
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void InitializeNative(IntPtr self, ref Desc desc);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr CreateInstanceNative(IntPtr self);

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
