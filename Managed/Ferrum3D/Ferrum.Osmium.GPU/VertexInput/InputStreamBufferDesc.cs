using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.VertexInput
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct InputStreamBufferDesc
    {
        public readonly int Stride;
        public readonly InputStreamRate InputRate;

        public InputStreamBufferDesc(int stride, InputStreamRate inputRate)
        {
            Stride = stride;
            InputRate = inputRate;
        }

        public InputStreamBufferDesc WithInputRate(InputStreamRate inputRate)
        {
            return new InputStreamBufferDesc(Stride, inputRate);
        }

        public InputStreamBufferDesc WithStride(int stride)
        {
            return new InputStreamBufferDesc(stride, InputRate);
        }

        public bool Equals(InputStreamBufferDesc other)
        {
            return Stride == other.Stride && InputRate == other.InputRate;
        }

        public override bool Equals(object obj)
        {
            return obj is InputStreamBufferDesc other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return ((int)Stride * 397) ^ (int)InputRate;
            }
        }

        public static bool operator ==(InputStreamBufferDesc lhs, InputStreamBufferDesc rhs)
        {
            return lhs.Equals(rhs);
        }

        public static bool operator !=(InputStreamBufferDesc lhs, InputStreamBufferDesc rhs)
        {
            return !(lhs == rhs);
        }
    }
}
