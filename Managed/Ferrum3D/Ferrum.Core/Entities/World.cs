﻿using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Entities
{
    public class World : IDisposable
    {
        public static IntPtr Handle { get; private set; }

        private static EntityRegistry registry;
        private readonly Dictionary<Type, ComponentSystem> systems = new();

        public World()
        {
            Handle = ConstructNative();
            registry = EntityRegistry.FromHandle(GetRegistryNative(Handle));
            RegisterCoreSystemsNative(Handle);
        }

        public void Dispose()
        {
            foreach (var system in systems.Values)
            {
                system.OnDestroy();
            }

            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        public void RegisterSystem<T>(T system)
            where T : ComponentSystem
        {
            if (systems.ContainsKey(typeof(T)))
            {
                throw new ArgumentException($"System of type {typeof(T)} was already registered");
            }

            systems[typeof(T)] = system;
            system.EntityRegistry = registry;
            RegisterSystemNative(Handle, system.Handle);
        }

        public void UnregisterSystem<T>()
        {
            if (!systems.TryGetValue(typeof(T), out var system))
            {
                throw new Exception($"System of type {typeof(T)} was not registered");
            }

            UnregisterSystemNative(Handle, system.Handle);
            systems.Remove(typeof(T));
        }

        private static void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
            Handle = IntPtr.Zero;
        }

        [DllImport("FeCoreBindings", EntryPoint = "World_Construct")]
        private static extern IntPtr ConstructNative();

        [DllImport("FeCoreBindings", EntryPoint = "World_Registry")]
        private static extern IntPtr GetRegistryNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "World_RegisterCoreSystems")]
        private static extern void RegisterCoreSystemsNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "World_RegisterSystem")]
        private static extern void RegisterSystemNative(IntPtr self, IntPtr system);

        [DllImport("FeCoreBindings", EntryPoint = "World_RegisterSystem")]
        private static extern void UnregisterSystemNative(IntPtr self, IntPtr system);

        [DllImport("FeCoreBindings", EntryPoint = "World_Destruct")]
        private static extern void DestructNative(IntPtr self);

        ~World()
        {
            ReleaseUnmanagedResources();
        }
    }
}