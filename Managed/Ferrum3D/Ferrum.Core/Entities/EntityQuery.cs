using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Entities
{
    public sealed class EntityQuery : UnmanagedObject
    {
        public IEnumerable<ComponentType> None
        {
            get => none;
            set => none = value.ToNativeArray();
        }

        public IEnumerable<ComponentType> All
        {
            get => all;
            set => all = value.ToNativeArray();
        }

        public IEnumerable<ComponentType> Any
        {
            get => any;
            set => any = value.ToNativeArray();
        }

        private NativeArray<ComponentType> none;
        private NativeArray<ComponentType> all;
        private NativeArray<ComponentType> any;

        internal EntityQuery(IntPtr registry) : base(ConstructNative(registry))
        {
        }

        public void Update()
        {
            NoneOfNative(Handle, none.DataPointer, none.Count);
            AllOfNative(Handle, all.DataPointer, all.Count);
            AnyOfNative(Handle, any.DataPointer, any.Count);

        }

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_Construct")]
        private static extern IntPtr ConstructNative(IntPtr registry);

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_NoneOf")]
        private static extern void NoneOfNative(IntPtr self, IntPtr componentTypes, int componentTypeCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_AllOf")]
        private static extern void AllOfNative(IntPtr self, IntPtr componentTypes, int componentTypeCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_AnyOf")]
        private static extern void AnyOfNative(IntPtr self, IntPtr componentTypes, int componentTypeCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_Destruct")]
        private static extern void DestructNative(IntPtr self);
    }
}
