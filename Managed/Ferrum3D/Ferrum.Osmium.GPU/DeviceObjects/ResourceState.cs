namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public enum ResourceState
    {
        Undefined,
        Common,
        VertexBuffer,
        ConstantBuffer,
        IndexBuffer,
        RenderTarget,
        UnorderedAccess,
        DepthWrite,
        DepthRead,
        ShaderResource,
        IndirectArgument,
        CopyDest,
        CopySource,
        Present
    }
}
