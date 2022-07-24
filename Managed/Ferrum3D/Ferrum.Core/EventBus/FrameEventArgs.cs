using System.Runtime.InteropServices;

namespace Ferrum.Core.EventBus
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct FrameEventArgs
    {
        public readonly uint FrameIndex;
        public readonly float DeltaTime;

        public FrameEventArgs(uint frameIndex, float deltaTime)
        {
            FrameIndex = frameIndex;
            DeltaTime = deltaTime;
        }
    }
}
