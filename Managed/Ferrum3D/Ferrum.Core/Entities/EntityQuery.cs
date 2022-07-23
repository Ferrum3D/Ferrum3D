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
        public IEnumerable<ArchetypeChunk> Chunks => chunks;

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

        private NativeArray<ArchetypeChunk> chunks;

        internal EntityQuery(IntPtr registry) : base(ConstructNative(registry))
        {
        }

        public void Update()
        {
            NoneOfNative(Handle, none.DataPointer, none.Count);
            AllOfNative(Handle, all.DataPointer, all.Count);
            AnyOfNative(Handle, any.DataPointer, any.Count);

            UpdateNative(Handle);
            GetChunksNative(Handle, out var chunkArrayHandle);
            chunks = NativeArray<ArchetypeChunk>.FromHandle(chunkArrayHandle);
        }

        public void ForEach<T1>(EntityQueryDelegate<T1> f)
            where T1 : unmanaged
        {
            for (var i = 0; i < chunks.Count; i++)
            {
                unsafe
                {
                    var p1 = chunks[i].OffsetOf<T1>();
                    for (var j = 0; j < chunks[i].Count; ++j)
                    {
                        f(ref p1[j]);
                    }
                }
            }
        }

        public void ForEach<T1, T2>(EntityQueryDelegate<T1, T2> f)
            where T1 : unmanaged
            where T2 : unmanaged
        {
            for (var i = 0; i < chunks.Count; i++)
            {
                unsafe
                {
                    var p1 = chunks[i].OffsetOf<T1>();
                    var p2 = chunks[i].OffsetOf<T2>();
                    
                    for (var j = 0; j < chunks[i].Count; ++j)
                    {
                        f(ref p1[j], ref p2[j]);
                    }
                }
            }
        }

        public void ForEach<T1, T2, T3>(EntityQueryDelegate<T1, T2, T3> f)
            where T1 : unmanaged
            where T2 : unmanaged
            where T3 : unmanaged
        {
            for (var i = 0; i < chunks.Count; i++)
            {
                unsafe
                {
                    var p1 = chunks[i].OffsetOf<T1>();
                    var p2 = chunks[i].OffsetOf<T2>();
                    var p3 = chunks[i].OffsetOf<T3>();

                    for (var j = 0; j < chunks[i].Count; ++j)
                    {
                        f(ref p1[j], ref p2[j], ref p3[j]);
                    }
                }
            }
        }

        public void ForEach<T1, T2, T3, T4>(EntityQueryDelegate<T1, T2, T3, T4> f)
            where T1 : unmanaged
            where T2 : unmanaged
            where T3 : unmanaged
            where T4 : unmanaged
        {
            for (var i = 0; i < chunks.Count; i++)
            {
                unsafe
                {
                    var p1 = chunks[i].OffsetOf<T1>();
                    var p2 = chunks[i].OffsetOf<T2>();
                    var p3 = chunks[i].OffsetOf<T3>();
                    var p4 = chunks[i].OffsetOf<T4>();
                    
                    for (var j = 0; j < chunks[i].Count; ++j)
                    {
                        f(ref p1[j], ref p2[j], ref p3[j], ref p4[j]);
                    }
                }
            }
        }

        protected override void ReleaseUnmanagedResources()
        {
            chunks.Dispose();
            any.Dispose();
            all.Dispose();
            none.Dispose();
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

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_Update")]
        private static extern void UpdateNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_GetChunks")]
        private static extern void GetChunksNative(IntPtr self, out ByteBuffer.Native result);

        [DllImport("FeCoreBindings", EntryPoint = "EntityQuery_Destruct")]
        private static extern void DestructNative(IntPtr self);
    }
}
