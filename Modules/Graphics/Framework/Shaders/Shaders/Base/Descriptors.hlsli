#pragma once
#include <Shaders/Base/BaseTypes.hlsli>

template<class TResource, class T, uint TDim>
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

    T Load(vector<uint, TDim + 1> location)
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
template<class T> struct Texture1DDescriptor : ImageSRVDescriptorBase<Texture1D<T>, T, 1> {};
template<class T> struct Texture2DDescriptor : ImageSRVDescriptorBase<Texture2D<T>, T, 2> {};
template<class T> struct Texture3DDescriptor : ImageSRVDescriptorBase<Texture3D<T>, T, 3> {};
template<class T> struct Texture1DArrayDescriptor : ImageSRVDescriptorBase<Texture1DArray<T>, T, 2> {};
template<class T> struct Texture2DArrayDescriptor : ImageSRVDescriptorBase<Texture2DArray<T>, T, 3> {};
template<class T> struct TextureCubeDescriptor : ImageSRVDescriptorBase<TextureCube<T>, T, 3> {};
template<class T> struct TextureCubeArrayDescriptor : ImageSRVDescriptorBase<Texture2DArray<T>, T, 4> {};
// clang-format on


template<class TResource, class T, uint TDim>
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
template<class T> struct RWTexture1DDescriptor : ImageUAVDescriptorBase<RWTexture1D<T>, T, 1> {};
template<class T> struct RWTexture2DDescriptor : ImageUAVDescriptorBase<RWTexture2D<T>, T, 2> {};
template<class T> struct RWTexture3DDescriptor : ImageUAVDescriptorBase<RWTexture3D<T>, T, 3> {};
template<class T> struct RWTexture1DArrayDescriptor : ImageUAVDescriptorBase<RWTexture1DArray<T>, T, 2> {};
template<class T> struct RWTexture2DArrayDescriptor : ImageUAVDescriptorBase<RWTexture2DArray<T>, T, 3> {};

template<class T> struct GloballyCoherentRWTexture1DDescriptor : ImageUAVDescriptorBase<fe_globallycoherent RWTexture1D<T>, T, 1> {};
template<class T> struct GloballyCoherentRWTexture2DDescriptor : ImageUAVDescriptorBase<fe_globallycoherent RWTexture2D<T>, T, 2> {};
template<class T> struct GloballyCoherentRWTexture3DDescriptor : ImageUAVDescriptorBase<fe_globallycoherent RWTexture3D<T>, T, 3> {};
template<class T> struct GloballyCoherentRWTexture1DArrayDescriptor : ImageUAVDescriptorBase<fe_globallycoherent RWTexture1DArray<T>, T, 2> {};
template<class T> struct GloballyCoherentRWTexture2DArrayDescriptor : ImageUAVDescriptorBase<fe_globallycoherent RWTexture2DArray<T>, T, 3> {};
// clang-format on


template<class TResource, class T>
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
template<class T> struct BufferDescriptor : BufferSRVDescriptorBase<Buffer<T>, T> {};
template<class T> struct StructuredBufferDescriptor : BufferSRVDescriptorBase<StructuredBuffer<T>, T> {};
// clang-format on


template<class TResource, class T>
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
template<class T> struct RWBufferDescriptor : BufferUAVDescriptorBase<RWBuffer<T>, T> {};
template<class T> struct RWStructuredBufferDescriptor : BufferUAVDescriptorBase<RWStructuredBuffer<T>, T> {};

template<class T> struct GloballyCoherentRWStructuredBufferDescriptor : BufferUAVDescriptorBase<fe_globallycoherent RWStructuredBuffer<T>, T> {};
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


#define FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(returnType, method, params, args)                                                      \
    returnType method params                                                                                                     \
    {                                                                                                                            \
        return Get().method args;                                                                                                \
    }


struct ByteAddressBufferDescriptor
{
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint, Load, (int address), (address))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint2, Load2, (int address), (address))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint3, Load3, (int address), (address))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint4, Load4, (int address), (address))

    template<class T>
    T Load(int address)
    {
        return Get().Load<T>(address);
    }

    ByteAddressBuffer Get()
    {
        ByteAddressBuffer resource = ResourceDescriptorHeap[NonUniformResourceIndex(m_index)];
        return resource;
    }

    uint m_index;
};


template<class TResource>
struct RWByteAddressBufferDescriptorBase
{
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint, Load, (int address), (address))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint2, Load2, (int address), (address))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint3, Load3, (int address), (address))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(uint4, Load4, (int address), (address))

    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, Store, (int address, uint value), (address, value))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, Store2, (int address, uint2 value), (address, value))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, Store3, (int address, uint3 value), (address, value))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, Store4, (int address, uint4 value), (address, value))

    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedAdd, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedAnd, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedOr, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedXor, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))

    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedMin, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedMax, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))

    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedExchange, (int address, uint value, out uint originalValue),
                                      (address, value, originalValue))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedCompareExchange,
                                      (int address, uint compareValue, uint value, out uint originalValue),
                                      (address, compareValue, value, originalValue))
    FE_MAP_BYTE_ADDRESS_BUFFER_METHOD(void, InterlockedCompareStore, (int address, uint compareValue, uint value),
                                      (address, compareValue, value))

    template<class T>
    T Load(int address)
    {
        return Get().template Load<T>(address);
    }

    template<class T>
    void Store(int address, T value)
    {
        Get().template Store<T>(address, value);
    }

    TResource Get()
    {
        TResource resource = ResourceDescriptorHeap[NonUniformResourceIndex(m_index)];
        return resource;
    }

    uint m_index;
};

// clang-format off
struct RWByteAddressBufferDescriptor : RWByteAddressBufferDescriptorBase<RWByteAddressBuffer> {};
struct GloballyCoherentRWByteAddressBufferDescriptor : RWByteAddressBufferDescriptorBase<fe_globallycoherent RWByteAddressBuffer> {};
// clang-format on

#undef FE_MAP_BYTE_ADDRESS_BUFFER_METHOD
