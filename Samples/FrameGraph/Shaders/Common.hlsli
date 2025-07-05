#pragma once

#define NUM_THREADS(x, y, z) [numthreads(x, y, z)]

static const uint kInvalidIndex = 0xffffffff;


template<typename TResource, typename T, uint TDim>
struct ImageSRVDescriptorBase
{
    T Sample(SamplerState s, vector<float, TDim> location)
    {
        return Get().Sample(s, location);
    }

    T SampleLevel(SamplerState s, vector<float, TDim> location, float lod)
    {
        return Get().SampleLevel(s, location, lod);
    }

    T SampleGrad(SamplerState s, vector<float, TDim> location, vector<float, TDim> ddx, vector<float, TDim> ddy)
    {
        return Get().SampleGrad(s, location, ddx, ddy);
    }

    T Load(vector<uint, TDim> location)
    {
        return Get().Load(location);
    }

    bool IsValid()
    {
        return m_index != kInvalidIndex;
    }

    TResource Get()
    {
        TResource resource = ResourceDescriptorHeap[NonUniformResourceIndex(m_index)];
        return resource;
    }

    uint m_index;
};


// clang-format off
template<typename T> struct Texture1DDescriptor : ImageSRVDescriptorBase<Texture1D<T>, T, 1> {};
template<typename T> struct Texture2DDescriptor : ImageSRVDescriptorBase<Texture2D<T>, T, 2> {};
template<typename T> struct Texture3DDescriptor : ImageSRVDescriptorBase<Texture3D<T>, T, 3> {};
template<typename T> struct Texture1DArrayDescriptor : ImageSRVDescriptorBase<Texture1DArray<T>, T, 2> {};
template<typename T> struct Texture2DArrayDescriptor : ImageSRVDescriptorBase<Texture2DArray<T>, T, 3> {};
template<typename T> struct TextureCubeDescriptor : ImageSRVDescriptorBase<TextureCube<T>, T, 3> {};
template<typename T> struct TextureCubeArrayDescriptor : ImageSRVDescriptorBase<Texture2DArray<T>, T, 4> {};
// clang-format on


template<typename TResource, typename T, uint TDim>
struct ImageUAVDescriptorBase
{
    void Store(vector<uint, TDim> location, T value)
    {
        Get()[location] = value;
    }

    T Load(vector<uint, TDim> location)
    {
        return Get().Load(location);
    }

    bool IsValid()
    {
        return m_index != kInvalidIndex;
    }

    TResource Get()
    {
        TResource resource = ResourceDescriptorHeap[NonUniformResourceIndex(m_index)];
        return resource;
    }

    uint m_index;
};


// clang-format off
template<typename StorageType> struct RWTexture1DDescriptor : ImageUAVDescriptorBase<RWTexture1D<StorageType>, StorageType, 1> {};
template<typename StorageType> struct RWTexture2DDescriptor : ImageUAVDescriptorBase<RWTexture2D<StorageType>, StorageType, 2> {};
template<typename StorageType> struct RWTexture3DDescriptor : ImageUAVDescriptorBase<RWTexture3D<StorageType>, StorageType, 3> {};
template<typename StorageType> struct RWTexture1DArrayDescriptor : ImageUAVDescriptorBase<RWTexture1DArray<StorageType>, StorageType, 2> {};
template<typename StorageType> struct RWTexture2DArrayDescriptor : ImageUAVDescriptorBase<RWTexture2DArray<StorageType>, StorageType, 3> {};
// clang-format on


template<typename TResource, typename T>
struct BufferSRVDescriptorBase
{
    T Load(uint index)
    {
        return Get()[index];
    }

    bool IsValid()
    {
        return m_index != kInvalidIndex;
    }

    TResource Get()
    {
        TResource resource = ResourceDescriptorHeap[NonUniformResourceIndex(m_index)];
        return resource;
    }

    uint m_index;
};


// clang-format off
template<typename T> struct BufferDescriptor : BufferSRVDescriptorBase<Buffer<T>, T> {};
template<typename T> struct StructuredBufferDescriptor : BufferSRVDescriptorBase<StructuredBuffer<T>, T> {};
// clang-format on


template<typename TResource, typename T>
struct BufferUAVDescriptorBase
{
    void Store(uint index, T value)
    {
        Get()[index] = value;
    }

    T Load(uint index)
    {
        return Get()[index];
    }

    bool IsValid()
    {
        return m_index != kInvalidIndex;
    }

    TResource Get()
    {
        TResource resource = ResourceDescriptorHeap[NonUniformResourceIndex(m_index)];
        return resource;
    }

    uint m_index;
};


// clang-format off
template<typename T> struct RWBufferDescriptor : BufferUAVDescriptorBase<RWBuffer<T>, T> {};
template<typename T> struct RWStructuredBufferDescriptor : BufferUAVDescriptorBase<RWStructuredBuffer<T>, T> {};
// clang-format on


struct SamplerDescriptor
{
    SamplerState Get()
    {
        return SamplerDescriptorHeap[NonUniformResourceIndex(m_index)];
    }

    bool IsValid()
    {
        return m_index != kInvalidIndex;
    }

    uint m_index;
};
