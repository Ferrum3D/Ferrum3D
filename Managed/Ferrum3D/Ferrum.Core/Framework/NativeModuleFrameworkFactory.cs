using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Framework
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate IntPtr CreateModuleInstanceNative(IntPtr environment);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void DestructModuleInstanceNative(IntPtr handle);
    
    [SuppressMessage("ReSharper", "StaticMemberInGenericType")]
    public abstract class NativeModuleFrameworkFactory<TModule> : IFrameworkFactory
    {
        public static bool IsLoaded { get; private set; }
        public static DynamicLibrary Library { get; private set; }
        private static FrameworkBase framework;
        private static IntPtr Handle { get; set; }
        private static string libraryName;

        public NativeModuleFrameworkFactory(string libraryName)
        {
            NativeModuleFrameworkFactory<TModule>.libraryName = libraryName + "Bindings";
        }

        public IFramework Load()
        {
            if (IsLoaded)
            {
                return framework;
            }

            IsLoaded = true;
            Library = DynamicLibrary.FromPath(libraryName);
            var createModuleInstance = Library.GetFunction<CreateModuleInstanceNative>("CreateModuleInstance");
            Handle = createModuleInstance(Engine.Environment);
            framework = CreateFramework(Handle);
            return framework;
        }

        public void Unload()
        {
            IsLoaded = false;
            framework.Dispose();
            var destructModuleInstance = Library.GetFunction<DestructModuleInstanceNative>("DestructModuleInstance");
            destructModuleInstance(Handle);
            Library.Dispose();
        }

        protected abstract FrameworkBase CreateFramework(IntPtr handle);
    }
}
