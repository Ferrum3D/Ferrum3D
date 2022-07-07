using System;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Framework
{
    public abstract class NativeModuleFramework : FrameworkBase
    {
        public string LibraryName { get; }
        public abstract DynamicLibrary Library { get; }
        protected IntPtr Handle;

        protected NativeModuleFramework(string libraryName, IntPtr handle)
        {
            LibraryName = libraryName;
            Handle = handle;
        }
    }
}
