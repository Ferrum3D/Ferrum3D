using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.EventBus
{
    public abstract class EventBusBase : UnmanagedObject
    {
        private readonly string prefix;
        private readonly DynamicLibrary moduleLibrary;

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr ConstructEventBusNative();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void DestructEventBusNative(IntPtr self);

        protected EventBusBase(string eventClassName, DynamicLibrary library = null)
            : base(IntPtr.Zero)
        {
            moduleLibrary = library ?? Engine.CoreModuleLibrary;
            prefix = $"EventBus_{eventClassName}_";
            Handle = GetEntryPoint<ConstructEventBusNative>("Construct")();
        }

        protected T GetEntryPoint<T>(string functionName)
            where T : Delegate
        {
            return moduleLibrary.GetFunction<T>(prefix + functionName);
        }

        protected T GetSendEventEntryPoint<T>(string eventName)
            where T : Delegate
        {
            return moduleLibrary.GetFunction<T>($"{prefix}SendEvent_{eventName}");
        }

        protected override void ReleaseUnmanagedResources()
        {
            GetEntryPoint<DestructEventBusNative>("Destruct")(Handle);
        }
    }
}
