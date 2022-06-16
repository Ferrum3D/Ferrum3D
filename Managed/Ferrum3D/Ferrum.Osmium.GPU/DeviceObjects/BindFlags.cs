namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public enum BindFlags
    {
        None             = 0,
        VertexBuffer     = 1 << 0,
        IndexBuffer      = 1 << 1,
        ConstantBuffer   = 1 << 2,
        ShaderResource   = 1 << 3,
        StreamOutput     = 1 << 4,
        RenderTarget     = 1 << 5,
        DepthStencil     = 1 << 6,
        UnorderedAccess  = 1 << 7,
        IndirectDrawArgs = 1 << 8,
        InputAttachment  = 1 << 9
    }
}
