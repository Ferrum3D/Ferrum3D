using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.Common
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct Scissor
    {
        public readonly int MinX;
        public readonly int MinY;
        
        public readonly int MaxX;
        public readonly int MaxY;

        public float Width => MaxX - MinX;

        public float Height => MaxY - MinY;

        public Scissor(int minX, int minY, int maxX, int maxY)
        {
            MinX = minX;
            MinY = minY;
            MaxX = maxX;
            MaxY = maxY;
        }
        
        public override string ToString()
        {
            return $"Scissor ({{{MinX}; {MinY}}} {{{MaxX}; {MaxY}}})";
        }
    }
}
