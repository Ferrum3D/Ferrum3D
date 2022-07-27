using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct SubpassAttachment
    {
        public static readonly SubpassAttachment None = new(ResourceState.Undefined, uint.MaxValue);
        public readonly ResourceState State;
        public readonly uint Index;

        public SubpassAttachment(ResourceState state, uint index)
        {
            Index = index;
            State = state;
        }
    }
}
