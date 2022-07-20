using System.Runtime.InteropServices;
using Ferrum.Core.Entities;
using Ferrum.Core.Math;

namespace Ferrum.Core.Components
{
    [StructLayout(LayoutKind.Sequential)]
    [Component("1F3CD11C-5547-4773-9941-082C257C6729", Alignment = 8, Unmanaged = true)]
    public readonly struct PositionComponent
    {
        public readonly float X;
        public readonly float Y;
        public readonly float Z;

        public PositionComponent(float x, float y, float z = 0.0f)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public PositionComponent(in Vector2F vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = 0;
        }

        public PositionComponent(in Vector3F vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
        }

        public PositionComponent(in Vector4F vector)
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
