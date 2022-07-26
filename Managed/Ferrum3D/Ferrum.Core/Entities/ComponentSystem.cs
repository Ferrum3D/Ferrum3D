using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using Ferrum.Core.EventBus;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Entities
{
    [SuppressMessage("ReSharper", "PrivateFieldCanBeConvertedToLocalVariable")]
    public abstract class ComponentSystem : UnmanagedObject
    {
        public EntityRegistry EntityRegistry { get; internal set; }

        private FrameEventArgs frameEventArgs;

        private readonly List<ComponentSubsystem> subsystems = new();

        protected float DeltaTime => frameEventArgs.DeltaTime;
        protected uint FrameIndex => frameEventArgs.FrameIndex;

        // These delegates must be stored in fields of the class to prevent GC to collect them before calls from unmanaged code
        private readonly CreateCallback createCallback;
        private readonly DestroyCallback destroyCallback;
        private readonly UpdateCallback updateCallback;

        protected ComponentSystem()
            : base(ConstructNative())
        {
            createCallback = OnCreate;
            destroyCallback = OnDestroyImpl;
            updateCallback = OnUpdateImpl;

            SetCreateCallbackNative(Handle, Marshal.GetFunctionPointerForDelegate(createCallback));
            SetDestroyCallbackNative(Handle, Marshal.GetFunctionPointerForDelegate(destroyCallback));
            SetUpdateCallbackNative(Handle, Marshal.GetFunctionPointerForDelegate(updateCallback));
        }

        protected void AddSubsystem(ComponentSubsystem subsystem)
        {
            subsystem.ParentSystem = this;
            subsystem.OnCreate();
            subsystems.Add(subsystem);
        }

        protected virtual void OnCreate()
        {
        }

        protected virtual void OnUpdate()
        {
        }

        protected virtual void OnDestroy()
        {
        }

        private void OnUpdateImpl(in FrameEventArgs eventArgs)
        {
            frameEventArgs = eventArgs;

            foreach (var subsystem in subsystems)
            {
                subsystem.OnUpdate();
            }

            OnUpdate();
        }

        private void OnDestroyImpl()
        {
            foreach (var subsystem in subsystems)
            {
                subsystem.OnDestroy();
            }

            OnDestroy();
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
