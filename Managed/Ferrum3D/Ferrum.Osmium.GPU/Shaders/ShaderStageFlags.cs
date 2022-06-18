namespace Ferrum.Osmium.GPU.Shaders
{
    public enum ShaderStageFlags : uint
    {
        None = 0,
        Vertex = 1 << (int)ShaderStage.Vertex,
        Pixel = 1 << (int)ShaderStage.Pixel,
        Hull = 1 << (int)ShaderStage.Hull,
        Domain = 1 << (int)ShaderStage.Domain,
        Geometry = 1 << (int)ShaderStage.Geometry,
        Compute = 1 << (int)ShaderStage.Compute,
        All = Vertex | Hull | Domain | Pixel | Geometry | Compute
    }
}
