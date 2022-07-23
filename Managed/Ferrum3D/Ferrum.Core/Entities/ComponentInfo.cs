using System;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace Ferrum.Core.Entities
{
    [SuppressMessage("ReSharper", "StaticMemberInGenericType")]
    internal static class ComponentInfo<T>
        where T : unmanaged
    {
        public static ComponentType ComponentType
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                Initialize();
                return componentType;
            }
        }

        public static bool IsUnmanaged { get; private set; }

        public static Guid Type => ComponentType.Type;
        public static uint Alignment => ComponentType.Alignment;
        public static uint DataSize => ComponentType.DataSize;

        private static ComponentType componentType;
        private static bool initialized;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static void Initialize()
        {
            if (initialized)
            {
                return;
            }

            var attribute = typeof(T).GetCustomAttribute<ComponentAttribute>();
            componentType = ComponentType.Create<T>(attribute.Type, attribute.Alignment);
            IsUnmanaged = attribute.Unmanaged;
            initialized = true;
        }
    }
}
