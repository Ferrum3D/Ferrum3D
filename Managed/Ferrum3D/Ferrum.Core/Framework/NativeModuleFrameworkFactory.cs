using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Framework
{
    [SuppressMessage("ReSharper", "StaticMemberInGenericType")]
    public sealed class NativeModuleFrameworkFactory<TModule> : IFrameworkFactory
    {
        public static bool IsLoaded;
        public static DynamicLibrary Library;
        private static string libraryName;
        private static IntPtr handle;

        public NativeModuleFrameworkFactory(string libraryName)
        {
            NativeModuleFrameworkFactory<TModule>.libraryName = libraryName + "Bindings";
        }

        public void Load()
        {
            if (IsLoaded)
            {
                return;
            }

            IsLoaded = true;
            Library = DynamicLibrary.FromPath(libraryName);
            var createModuleInstance = Library.GetFunction<CreateModuleInstanceNative>("CreateModuleInstance");
            handle = createModuleInstance(Engine.Environment);
        }

        public void Unload()
        {
            IsLoaded = false;
            var destructModuleInstance = Library.GetFunction<DestructModuleInstanceNative>("DestructModuleInstance");
            destructModuleInstance(handle);
            Library.Dispose();
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr CreateModuleInstanceNative(IntPtr environment);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void DestructModuleInstanceNative(IntPtr handle);
    }
}
