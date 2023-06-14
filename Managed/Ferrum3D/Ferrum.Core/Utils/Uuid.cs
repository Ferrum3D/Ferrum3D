using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Ferrum.Core.Utils;

[StructLayout(LayoutKind.Sequential)]
public partial struct Uuid : IEquatable<Uuid>
{
    public static Uuid Empty { get; } = new();

    private unsafe fixed long data[2];

    private static readonly int stringLength = Guid.Empty.ToString().Length;

    public static Uuid Parse(string s)
    {
        UUID_FromString(s, out var result);
        return result;
    }

    public static Uuid FromGuid(in Guid guid)
    {
        UUID_FromGUID(in guid, out var result);
        return result;
    }

    public override string ToString()
    {
        unsafe
        {
            var buffer = stackalloc byte[stringLength];
            UUID_ToString(in this, new nint(buffer));
            return Encoding.ASCII.GetString(buffer, stringLength);
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Equals(Uuid other)
    {
        unsafe
        {
            return data[0] == other.data[0] && data[1] == other.data[1];
        }
    }

    public override bool Equals(object? obj)
    {
        return (obj is Uuid u && Equals(u)) ||
               (obj is Guid g && Equals(FromGuid(g)));
    }

    public override int GetHashCode()
    {
        return UUID_GetHash(in this).GetHashCode();
    }

    public static bool operator ==(Uuid left, Guid right)
    {
        return left == FromGuid(right);
    }

    public static bool operator !=(Uuid left, Guid right)
    {
        return left != FromGuid(right);
    }

    public static bool operator ==(Uuid left, Uuid right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(Uuid left, Uuid right)
    {
        return !left.Equals(right);
    }

    [DllImport("FeCoreBindings")]
    private static extern void UUID_FromGUID(in Guid guid, out Uuid result);

    [LibraryImport("FeCoreBindings", StringMarshalling = StringMarshalling.Utf8)]
    private static partial void UUID_FromString(string s, out Uuid result);

    [LibraryImport("FeCoreBindings")]
    private static partial void UUID_ToString(in Uuid self, nint result);

    [LibraryImport("FeCoreBindings")]
    private static partial ulong UUID_GetHash(in Uuid self);
}
