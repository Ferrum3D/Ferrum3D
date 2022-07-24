namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public enum ShaderResourceType
    {
        None,
        ConstantBuffer,
        TextureSrv,
        TextureUav,
        BufferSrv,
        BufferUav,
        Sampler,
        InputAttachment,
        Count
    }
}
