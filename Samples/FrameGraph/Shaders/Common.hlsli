#pragma once

static const uint kInvalidIndex = 0xffffffff;


template<class TResource, int TDummyArg>
TResource GetResourceFromDescriptorHeap(const in uint index)
{
    TResource resource = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    return resource;
}


struct DescriptorBase
{
    uint m_index;

    bool IsValid()
    {
        return m_index != kInvalidIndex;
    }
};


struct SamplerDescriptor : DescriptorBase
{
    SamplerState Get()
    {
        SamplerState sampler = SamplerDescriptorHeap[NonUniformResourceIndex(m_index)];
        return sampler;
    }
};


struct ImageSRVDescriptor : DescriptorBase
{
    template<class T>
    Texture1D<T> Get1D()
    {
        return GetResourceFromDescriptorHeap<Texture1D<T>, 0>(m_index);
    }

    template<class T>
    Texture2D<T> Get2D()
    {
        return GetResourceFromDescriptorHeap<Texture2D<T>, 0>(m_index);
    }

    template<class T>
    Texture3D<T> Get3D()
    {
        return GetResourceFromDescriptorHeap<Texture3D<T>, 0>(m_index);
    }
};


struct ImageUAVDescriptor : DescriptorBase
{
    template<class T>
    RWTexture1D<T> Get1D()
    {
        return GetResourceFromDescriptorHeap<RWTexture1D<T>, 0>(m_index);
    }

    template<class T>
    RWTexture2D<T> Get2D()
    {
        return GetResourceFromDescriptorHeap<RWTexture2D<T>, 0>(m_index);
    }

    template<class T>
    RWTexture3D<T> Get3D()
    {
        return GetResourceFromDescriptorHeap<RWTexture3D<T>, 0>(m_index);
    }
};


struct BufferSRVDescriptor : DescriptorBase
{
    template<class T>
    StructuredBuffer<T> Get()
    {
        return GetResourceFromDescriptorHeap<StructuredBuffer<T>, 0>(m_index);
    }
};


struct BufferUAVDescriptor : DescriptorBase
{
    template<class T>
    RWStructuredBuffer<T> Get()
    {
        return GetResourceFromDescriptorHeap<RWStructuredBuffer<T>, 0>(m_index);
    }
};


#define NUM_THREADS(x, y, z) [numthreads(x, y, z)]
