using System;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Entities
{
    public sealed partial class EntityRegistry : UnmanagedObject
    {
        public EntityRegistry() : base(ConstructNative())
        {
        }

        private EntityRegistry(IntPtr handle) : base(handle)
        {
        }

        public static EntityRegistry FromHandle(IntPtr handle)
        {
            return new EntityRegistry(handle);
        }

        public uint GetCurrentEntityVersion(Entity entity)
        {
            return GetCurrentEntityVersionNative(Handle, in entity);
        }

        public bool IsValid(Entity entity)
        {
            return IsValidNative(Handle, in entity);
        }

        public bool AddComponent<T>(Entity entity)
            where T : unmanaged
        {
            return AddComponentNative(Handle, in entity, ComponentInfo<T>.ComponentType);
        }

        public void AddComponent<T>(in NativeArray<Entity> entities)
            where T : unmanaged
        {
            AddComponentToEntitiesNative(Handle, entities.DataPointer, entities.Count, ComponentInfo<T>.ComponentType);
        }

        public bool AddComponent<T>(Entity entity, in T value)
            where T : unmanaged
        {
            var result = AddComponent<T>(entity);
            SetComponent(entity, value);
            return result;
        }

        public bool RemoveComponent<T>(Entity entity)
            where T : unmanaged
        {
            return RemoveComponentNative(Handle, in entity, ComponentInfo<T>.ComponentType);
        }

        public void RemoveComponent<T>(in NativeArray<Entity> entities)
            where T : unmanaged
        {
            RemoveComponentToEntitiesNative(Handle, entities.DataPointer, entities.Count, ComponentInfo<T>.ComponentType);
        }

        public bool HasComponent<T>(Entity entity)
            where T : unmanaged
        {
            return HasComponentNative(Handle, in entity, ComponentInfo<T>.ComponentType);
        }

        public bool SetComponent<T>(Entity entity, in T value)
            where T : unmanaged
        {
            unsafe
            {
                fixed (T* ptr = &value)
                {
                    return UpdateComponentNative(Handle, in entity, ComponentInfo<T>.ComponentType, new IntPtr(ptr));
                }
            }
        }

        public bool TryGetComponent<T>(Entity entity, out T value)
            where T : unmanaged
        {
            unsafe
            {
                fixed (T* ptr = &value)
                {
                    return CopyComponentNative(Handle, in entity, ComponentInfo<T>.ComponentType, new IntPtr(ptr));
                }
            }
        }

        public T GetComponent<T>(Entity entity)
            where T : unmanaged
        {
            if (!TryGetComponent<T>(entity, out var value))
            {
                throw new ArgumentException("Component not found in entity");
            }

            return value;
        }

        public Entity CreateEntity()
        {
            CreateEntityNative(Handle, out var entity);
            return entity;
        }

        public Entity CreateEntity(ComponentType[] componentTypes)
        {
            unsafe
            {
                fixed (ComponentType* ptr = componentTypes)
                {
                    CreateEntityWithComponentsNative(Handle, new IntPtr(ptr), componentTypes.Length, out var entity);
                    return entity;
                }
            }
        }

        public Entity CloneEntity(Entity entity)
        {
            CloneEntityNative(Handle, in entity, out var result);
            return result;
        }

        public void CreateEntities(in NativeArray<Entity> entities)
        {
            CreateEntitiesNative(Handle, entities.DataPointer, entities.Count);
        }

        public NativeArray<Entity> CreateEntities(int count)
        {
            var result = new NativeArray<Entity>(count);
            CreateEntities(result);
            return result;
        }

        public void CreateEntities(in NativeArray<Entity> entities, ComponentType[] componentTypes)
        {
            unsafe
            {
                fixed (ComponentType* ptr = componentTypes)
                {
                    CreateEntitiesWithComponentsNative(Handle, new IntPtr(ptr), componentTypes.Length,
                        entities.DataPointer, entities.Count);
                }
            }
        }

        public void CloneEntity(Entity entity, ref NativeArray<Entity> entities)
        {
            CloneEntityToArrayNative(Handle, in entity, entities.DataPointer, entities.Count);
        }

        public NativeArray<Entity> CloneEntity(Entity entity, int count)
        {
            var result = new NativeArray<Entity>(count);
            CloneEntity(entity, ref result);
            return result;
        }

        public void DestroyEntity(Entity entity)
        {
            DestroyEntityNative(Handle, in entity);
        }

        public void DestroyEntities(in NativeArray<Entity> entities)
        {
            DestroyEntitiesNative(Handle, entities.DataPointer, entities.Count);
        }

        public EntityQuery CreateQuery()
        {
            return new EntityQuery(Handle);
        }

        public EntityQuery ForEach<T1>(EntityQueryDelegate<T1> f, EntityQuery query = null)
            where T1 : unmanaged
        {
            if (query is null)
            {
                query = CreateQuery();
                query.All = ComponentType.CreateList<T1>();
            }

            query.Update();
            query.ForEach(f);
            return query;
        }

        public EntityQuery ForEach<T1, T2>(EntityQueryDelegate<T1, T2> f, EntityQuery query = null)
            where T1 : unmanaged
            where T2 : unmanaged
        {
            query ??= CreateQuery();
            query.All = ComponentType.CreateList<T1, T2>();
            query.Update();
            query.ForEach(f);
            return query;
        }

        public EntityQuery ForEach<T1, T2, T3>(EntityQueryDelegate<T1, T2, T3> f, EntityQuery query = null)
            where T1 : unmanaged
            where T2 : unmanaged
            where T3 : unmanaged
        {
            query ??= CreateQuery();
            query.All = ComponentType.CreateList<T1, T2, T3>();
            query.Update();
            query.ForEach(f);
            return query;
        }

        public EntityQuery ForEach<T1, T2, T3, T4>(EntityQueryDelegate<T1, T2, T3, T4> f, EntityQuery query = null)
            where T1 : unmanaged
            where T2 : unmanaged
            where T3 : unmanaged
            where T4 : unmanaged
        {
            query ??= CreateQuery();
            query.All = ComponentType.CreateList<T1, T2, T3, T4>();
            query.Update();
            query.ForEach(f);
            return query;
        }

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
