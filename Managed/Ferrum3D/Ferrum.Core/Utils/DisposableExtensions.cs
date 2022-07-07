using System;
using System.Linq;
using System.Reflection;

namespace Ferrum.Core.Utils
{
    public static class DisposableExtensions
    {
        /// <summary>
        ///     Call <see cref="IDisposable.Dispose"/> on all fields of a class.
        /// </summary>
        /// <param name="disposable">An instance of a class that implements <see cref="IDisposable"/>.</param>
        /// <typeparam name="T">Type of the class.</typeparam>
        public static void DisposeFields<T>(this T disposable)
            where T : IDisposable
        {
            const BindingFlags bindingFlags = BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance;
            foreach (var field in typeof(T).GetFields(bindingFlags).Reverse())
            {
                switch (field.GetValue(disposable))
                {
                    case IDisposable value:
                        value.Dispose();
                        break;
                    default:
                        continue;
                }
            }
        }
    }
}
