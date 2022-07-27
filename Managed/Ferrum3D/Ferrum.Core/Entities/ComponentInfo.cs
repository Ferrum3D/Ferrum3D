using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.CompilerServices;
using Ferrum.Core.Utils;

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

        public static Uuid Type => ComponentType.Type;
        public static uint Alignment => ComponentType.Alignment;
        public static uint DataSize => ComponentType.DataSize;

        public static bool IsUnmanaged { get; private set; }

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
            var alignment = attribute.Alignment == 0
                ? NativeUtils.AlignOf<T>()
                : attribute.Alignment;
            componentType = ComponentType.Create<T>(attribute.Type, alignment);
            IsUnmanaged = attribute.Unmanaged;
            initialized = true;
        }
    }
}
