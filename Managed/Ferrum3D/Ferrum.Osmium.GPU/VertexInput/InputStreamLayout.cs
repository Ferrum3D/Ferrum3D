using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.GPU.VertexInput
{
    public sealed class InputStreamLayout
    {
        public IReadOnlyList<InputStreamBufferDesc> Buffers => buffers;
        public IReadOnlyList<InputStreamAttributeDesc> Attributes => attributes;

        public PrimitiveTopology Topology { get; private set; }
        private readonly List<InputStreamBufferDesc> buffers = new List<InputStreamBufferDesc>();
        private readonly List<InputStreamAttributeDesc> attributes = new List<InputStreamAttributeDesc>();

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            return obj.GetType() == GetType() && Equals((InputStreamLayout)obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (buffers.GetHashCode() * 397) ^ attributes.GetHashCode();
            }
        }

        public static bool operator ==(InputStreamLayout left, InputStreamLayout right)
        {
            return Equals(left, right);
        }

        public static bool operator !=(InputStreamLayout left, InputStreamLayout right)
        {
            return !Equals(left, right);
        }

        private bool Equals(InputStreamLayout other)
        {
            return buffers.SequenceEqual(other.buffers) && attributes.SequenceEqual(other.attributes);
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly IntPtr Buffers;
            public readonly IntPtr Attributes;
            public readonly PrimitiveTopology Topology;

            public Native(InputStreamLayout layout)
            {
                Buffers = ByteBuffer.FromCollection(layout.buffers).Detach();
                Attributes = ByteBuffer.FromCollection(layout.attributes
                    .Select(x => new InputStreamAttributeDesc.Native(x))
                    .ToArray()).Detach();
                Topology = layout.Topology;
            }
        }

        public class BufferBuilder
        {
            internal readonly List<InputStreamAttributeDesc> Attributes = new List<InputStreamAttributeDesc>();
            internal InputStreamBufferDesc Buffer;
            internal uint Offset;
            private readonly Builder parent;

            private readonly uint index;

            internal BufferBuilder(InputStreamRate inputRate, uint index, Builder parent)
            {
                Buffer = Buffer.WithInputRate(inputRate);
                this.index = index;
                this.parent = parent;
            }

            public BufferBuilder AddAttribute(Format format, string semantic)
            {
                Attributes.Add(new InputStreamAttributeDesc(semantic, index, Offset, format));
                Offset += format.GetSize();
                return this;
            }

            public Builder Build()
            {
                return parent;
            }
        }

        public class Builder
        {
            private readonly PrimitiveTopology topology;
            private readonly List<BufferBuilder> buffers = new List<BufferBuilder>();

            public Builder(PrimitiveTopology topology = PrimitiveTopology.TriangleList)
            {
                this.topology = topology;
            }

            public BufferBuilder AddBuffer(InputStreamRate inputRate)
            {
                buffers.Add(new BufferBuilder(inputRate, (uint)buffers.Count, this));
                return buffers.Last();
            }

            public InputStreamLayout Build()
            {
                var result = new InputStreamLayout
                {
                    Topology = topology
                };

                foreach (var bufferBuilder in buffers)
                {
                    result.buffers.Add(bufferBuilder.Buffer.WithStride(bufferBuilder.Offset));

                    foreach (var attribute in bufferBuilder.Attributes)
                    {
                        result.attributes.Add(attribute);
                    }
                }

                return result;
            }
        }
    }
}
