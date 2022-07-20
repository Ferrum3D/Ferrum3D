using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Entities
{
    public sealed partial class EntityRegistry
    {
        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_Construct")]
        private static extern IntPtr ConstructNative();

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_GetCurrentEntityVersion")]
        private static extern uint GetCurrentEntityVersionNative(IntPtr self, in Entity entity);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_IsValid")]
        private static extern bool IsValidNative(IntPtr self, in Entity entity);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_AddComponent")]
        private static extern bool AddComponentNative(IntPtr self, in Entity entity, in ComponentType componentType);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_AddComponentToEntities")]
        private static extern bool AddComponentToEntitiesNative(IntPtr self, IntPtr entities, int entityCount,
            in ComponentType componentType);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_RemoveComponent")]
        private static extern bool RemoveComponentNative(IntPtr self, in Entity entity, in ComponentType componentType);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_RemoveComponentToEntities")]
        private static extern bool RemoveComponentToEntitiesNative(IntPtr self, IntPtr entities, int entityCount,
            in ComponentType componentType);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CreateEntity")]
        private static extern void CreateEntityNative(IntPtr self, out Entity entity);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CreateEntityWithComponents")]
        private static extern void CreateEntityWithComponentsNative(IntPtr self, IntPtr componentTypes, int componentTypeCount,
            out Entity entity);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CloneEntity")]
        private static extern void CloneEntityNative(IntPtr self, in Entity entity, out Entity result);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CreateEntities")]
        private static extern void CreateEntitiesNative(IntPtr self, IntPtr entities, int entityCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CreateEntitiesWithComponents")]
        private static extern void CreateEntitiesWithComponentsNative(IntPtr self, IntPtr componentTypes, int componentTypeCount,
            IntPtr entities, int entityCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CloneEntityToArray")]
        private static extern void CloneEntityToArrayNative(IntPtr self, in Entity entity, IntPtr entities, int entityCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_DestroyEntity")]
        private static extern void DestroyEntityNative(IntPtr self, in Entity entity);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_DestroyEntities")]
        private static extern void DestroyEntitiesNative(IntPtr self, IntPtr entities, int entityCount);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_HasComponent")]
        private static extern bool HasComponentNative(IntPtr self, in Entity entity, in ComponentType componentType);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_CopyComponent")]
        private static extern bool CopyComponentNative(IntPtr self, in Entity entity, in ComponentType componentType,
            IntPtr destination);

        [DllImport("FeCoreBindings", EntryPoint = "EntityRegistry_UpdateComponent")]
        private static extern bool UpdateComponentNative(IntPtr self, in Entity entity, in ComponentType componentType,
            IntPtr source);
    }
}
