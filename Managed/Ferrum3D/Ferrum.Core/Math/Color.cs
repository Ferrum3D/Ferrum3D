using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Color
    {
        private Vector4F color;
        private const float MaxByte = 255.0f;
        private const float InvMaxByte = 1.0f / 255.0f;

        public Color(Vector4F rgba)
        {
            color = rgba;
        }

        public Color(float r, float g, float b, float a) : this(new Vector4F(r, g, b, a))
        {
        }

        public float this[int index]
        {
            get => color[index];
            set => color[index] = value;
        }

        public static Color GetZero()
        {
            return new Color(0, 0, 0, 0);
        }

        public static Color FromBytes(byte r, byte g, byte b, byte a)
        {
            var red = r * InvMaxByte;
            var green = g * InvMaxByte;
            var blue = b * InvMaxByte;
            var alpha = a * InvMaxByte;
            return new Color(red, green, blue, alpha);
        }

        public static Color FromUInt32(uint rgba)
        {
            var r = ((rgba >> 24) & 0xFF) * InvMaxByte;
            var g = ((rgba >> 16) & 0xFF) * InvMaxByte;
            var b = ((rgba >> 8) & 0xFF) * InvMaxByte;
            var a = ((rgba >> 0) & 0xFF) * InvMaxByte;

            return new Color(r, g, b, a);
        }
    }
}
