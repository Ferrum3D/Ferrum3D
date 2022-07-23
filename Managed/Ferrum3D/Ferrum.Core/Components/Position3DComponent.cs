using System.Runtime.InteropServices;
using Ferrum.Core.Entities;
using Ferrum.Core.Math;

namespace Ferrum.Core.Components
{
    [StructLayout(LayoutKind.Sequential)]
    [Component("930DFC8B-1340-4C39-A0A9-CD44F0E2E5D6", Unmanaged = true)]
    public readonly struct Position2DComponent
    {
        public readonly float X;
        public readonly float Y;

        public Position2DComponent(float x, float y)
        {
            X = x;
            Y = y;
        }

        public Position2DComponent(in Vector2F vector)
        {
            X = vector.X;
            Y = vector.Y;
        }

        public Position2DComponent(in Vector3F vector)
        {
            X = vector.X;
            Y = vector.Y;
        }

        public Position2DComponent(in Vector4F vector)
        {
            X = vector.X;
            Y = vector.Y;
        }

        public Vector2F AsVector2F()
        {
            return new Vector2F(X, Y);
        }

        public Vector3F AsVector3F()
        {
            return new Vector3F(X, Y, 0);
        }

        public Vector4F AsVector4F()
        {
            return new Vector4F(X, Y, 0, 0);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    [Component("1F3CD11C-5547-4773-9941-082C257C6729", Unmanaged = true)]
    public readonly struct Position3DComponent
    {
        public readonly float X;
        public readonly float Y;
        public readonly float Z;

        public Position3DComponent(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public Position3DComponent(in Vector2F vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = 0;
        }

        public Position3DComponent(in Vector3F vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
        }

        public Position3DComponent(in Vector4F vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
        }

        public Vector2F AsVector2F()
        {
            return new Vector2F(X, Y);
        }

        public Vector3F AsVector3F()
        {
            return new Vector3F(X, Y, Z);
        }

        public Vector4F AsVector4F()
        {
            return new Vector4F(X, Y, Z, 0);
        }
    }
}
