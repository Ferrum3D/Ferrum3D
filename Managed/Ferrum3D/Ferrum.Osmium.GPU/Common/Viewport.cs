using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.Common
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct Viewport
    {
        public readonly float MinX;
        public readonly float MinY;
        public readonly float MinZ;

        public readonly float MaxX;
        public readonly float MaxY;
        public readonly float MaxZ;

        public float Width => MaxX - MinX;

        public float Height => MaxY - MinY;

        public float Depth => MaxZ - MinZ;

        public Viewport(float minX, float maxX, float minY, float maxY, float minZ = 0, float maxZ = 1)
        {
            MinX = minX;
            MinY = minY;
            MinZ = minZ;
            MaxX = maxX;
            MaxY = maxY;
            MaxZ = maxZ;
        }

        public override string ToString()
        {
            return $"Viewport ({{{MinX}; {MinY}; {MinZ}}} {{{MaxX}; {MaxY}; {MaxZ}}})";
        }
    }
}
