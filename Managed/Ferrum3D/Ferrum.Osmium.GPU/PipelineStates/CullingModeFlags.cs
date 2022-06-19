namespace Ferrum.Osmium.GPU.PipelineStates
{
    public enum CullingModeFlags
    {
        None = 0,
        Back = 1 << CullingMode.Back,
        Front = 1 << CullingMode.Front,
        BackAndFront = Back | Front
    }
}
