namespace Ferrum.Core.EventBus
{
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
