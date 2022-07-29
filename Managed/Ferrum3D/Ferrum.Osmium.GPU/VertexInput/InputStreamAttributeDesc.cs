using System.Runtime.InteropServices;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU.VertexInput
{
    public readonly struct InputStreamAttributeDesc
    {
        public readonly string ShaderSemantic;
        public readonly int BufferIndex;
        public readonly int Offset;
        public readonly Format ElementFormat;

        public InputStreamAttributeDesc(string shaderSemantic, int bufferIndex, int offset, Format elementFormat)
        {
            ShaderSemantic = shaderSemantic;
            BufferIndex = bufferIndex;
            Offset = offset;
            ElementFormat = elementFormat;
        }

        public bool Equals(InputStreamAttributeDesc other)
        {
            return ShaderSemantic == other.ShaderSemantic
                   && BufferIndex == other.BufferIndex
                   && Offset == other.Offset
                   && ElementFormat == other.ElementFormat;
        }

        public override bool Equals(object obj)
        {
            return obj is InputStreamAttributeDesc other && Equals(other);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = ShaderSemantic != null ? ShaderSemantic.GetHashCode() : 0;
                hashCode = (hashCode * 397) ^ (int)BufferIndex;
                hashCode = (hashCode * 397) ^ (int)Offset;
                hashCode = (hashCode * 397) ^ (int)ElementFormat;
                return hashCode;
            }
        }

        public static bool operator ==(InputStreamAttributeDesc left, InputStreamAttributeDesc right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(InputStreamAttributeDesc left, InputStreamAttributeDesc right)
        {
            return !left.Equals(right);
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public readonly int BufferIndex;
            public readonly int Offset;
            public readonly Format ElementFormat;
            public unsafe fixed byte ShaderSemantic[32];

            public Native(InputStreamAttributeDesc desc)
            {
                for (var i = 0; i < desc.ShaderSemantic.Length; i++)
                {
                    unsafe
                    {
                        ShaderSemantic[i] = (byte)desc.ShaderSemantic[i];
                    }
                }

                BufferIndex = desc.BufferIndex;
                Offset = desc.Offset;
                ElementFormat = desc.ElementFormat;
            }
        }
    }
}
