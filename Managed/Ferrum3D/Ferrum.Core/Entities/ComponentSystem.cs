using System;
using System.Runtime.InteropServices;
using Ferrum.Core.EventBus;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Entities
{
    public abstract class ComponentSystem : UnmanagedObject
    {
        public EntityRegistry EntityRegistry { get; internal set; }

        internal FrameEventArgs frameEventArgs;

        protected float DeltaTime => frameEventArgs.DeltaTime;
        protected uint FrameIndex => frameEventArgs.FrameIndex;

        protected ComponentSystem()
            : base(ConstructNative())
        {
            SetCreateCallbackNative(Handle, Marshal.GetFunctionPointerForDelegate(new CreateCallback(OnCreate)));
            SetDestroyCallbackNative(Handle, Marshal.GetFunctionPointerForDelegate(new DestroyCallback(OnDestroy)));
            SetUpdateCallbackNative(Handle, Marshal.GetFunctionPointerForDelegate(new UpdateCallback((in FrameEventArgs args) =>
            {
                frameEventArgs = args;
                OnUpdate();
            })));
        }

        public virtual void OnCreate()
        {
        }

        public virtual void OnUpdate()
        {
        }

        public virtual void OnDestroy()
        {
        }

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void CreateCallback();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void UpdateCallback(in FrameEventArgs frameEventArgs);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void DestroyCallback();

        [DllImport("FeCoreBindings", EntryPoint = "CallbackSystem_Construct")]
        private static extern IntPtr ConstructNative();

        [DllImport("FeCoreBindings", EntryPoint = "CallbackSystem_SetCreateCallback")]
        private static extern void SetCreateCallbackNative(IntPtr self, IntPtr callback);

        [DllImport("FeCoreBindings", EntryPoint = "CallbackSystem_SetUpdateCallback")]
        private static extern void SetUpdateCallbackNative(IntPtr self, IntPtr callback);

        [DllImport("FeCoreBindings", EntryPoint = "CallbackSystem_SetDestroyCallback")]
        private static extern void SetDestroyCallbackNative(IntPtr self, IntPtr callback);

        [DllImport("FeCoreBindings", EntryPoint = "CallbackSystem_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
