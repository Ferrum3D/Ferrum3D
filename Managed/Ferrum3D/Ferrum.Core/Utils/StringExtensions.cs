using System;

namespace Ferrum.Core.Utils
{
    public static class StringExtensions
    {
        public static string Capitalize(this string value)
        {
            return value switch
            {
                null => throw new ArgumentNullException(nameof(value)),
                "" => throw new ArgumentException($"{nameof(value)} cannot be empty", nameof(value)),
                _ => value[0].ToString().ToUpper() + value.Substring(1)
            };
        }
    }
}
