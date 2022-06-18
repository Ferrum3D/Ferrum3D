using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct SubpassAttachment
    {
        public readonly ResourceState State;
        public readonly uint Index;

        public static readonly SubpassAttachment None = new SubpassAttachment(ResourceState.Undefined, uint.MaxValue);

        public SubpassAttachment(ResourceState state, uint index)
        {
            Index = index;
            State = state;
        }
    }
}
