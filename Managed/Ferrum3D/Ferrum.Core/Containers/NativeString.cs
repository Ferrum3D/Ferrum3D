using System;
using System.Linq;
using System.Text;
using Ferrum.Core.Utils;

namespace Ferrum.Core.Containers
{
    public struct NativeString : IDisposable
    {
        public readonly IntPtr DataPointer => characters.DataPointer;
        public readonly int Size => characters.Count;

        private NativeArray<byte> characters;

        public NativeString(string s)
        {
            var bytes = Encoding.Unicode.GetBytes(s);
            bytes = Encoding.Convert(Encoding.Unicode, Encoding.UTF8, bytes)
                .Append((byte)0)
                .ToArray();
            characters = new NativeArray<byte>(bytes);
        }

        public readonly override string ToString()
        {
            var bytes = Encoding.Convert(Encoding.UTF8, Encoding.Unicode, characters.ToArray());
            return Encoding.Unicode.GetString(bytes);
        }

        public void Dispose()
        {
            characters.Dispose();
        }
    }
}
