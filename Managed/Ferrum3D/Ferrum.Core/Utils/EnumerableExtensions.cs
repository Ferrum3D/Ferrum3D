using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Containers;

namespace Ferrum.Core.Utils
{
    public static class EnumerableExtensions
    {
        public static NativeArray<T> ToNativeArray<T>(this IEnumerable<T> source)
            where T : unmanaged
        {
            if (source is IReadOnlyList<T> list)
            {
                return new NativeArray<T>(list);
            }

            var array = source.ToArray();
            return new NativeArray<T>(array);
        }
    }
}
