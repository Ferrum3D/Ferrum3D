#pragma once

#define FE_HOST_BEGIN_NAMESPACE(name)
#define FE_HOST_END_NAMESPACE

#define FE_RTTI(...)
#define FE_RTTI_Reflect(...)
#define FE_DECLARE_PASS_DATA(...)

#define final
#define FE_INIT(...)

#if defined(__JETBRAINS_IDE__) || defined(__INTELLISENSE__)
typedef uint uint32_t;
typedef uint2 uint64_t;
#    define globallycoherent

#    define ms_payload
#    define ms_vertices
#    define ms_indices
#    define ms_primitives

#    define FE_CONST const

#    define FE_PLATFORM_VULKAN 1

#    define select(c, a, b) ((c) ? (a) : (b))
#else
#    define ms_payload payload
#    define ms_vertices vertices
#    define ms_indices indices
#    define ms_primitives primitives

#    define FE_CONST
#endif

#if FE_PLATFORM_VULKAN
#    include <Shaders/Platform/Vulkan/Intrinsics.hlsli>
#endif

struct BufferPointer
{
#if FE_PLATFORM_VULKAN

#    if defined(__JETBRAINS_IDE__) || defined(__INTELLISENSE__)
    uint64_t m_deviceAddress;

    uint64_t GetDeviceAddress() FE_CONST
    {
        return m_deviceAddress;
    }

    void SetDeviceAddress(const uint64_t deviceAddress)
    {
        m_deviceAddress = deviceAddress;
    }
#    else
    uint2 m_deviceAddress;

    uint64_t GetDeviceAddress() FE_CONST
    {
        return uint64_t(m_deviceAddress.x) | (uint64_t(m_deviceAddress.y) << 32);
    }

    void SetDeviceAddress(const uint64_t deviceAddress)
    {
        m_deviceAddress = uint2(uint32_t(deviceAddress), uint32_t(deviceAddress >> 32));
    }
#    endif

    template<typename T>
    T Read(uint32_t byteOffset) FE_CONST
    {
        return vk::RawBufferLoad<T>(NonUniformResourceIndex(GetDeviceAddress() + byteOffset));
    }

#else
#    error Not implemented
#endif
};

#define FE_CONSTEXPR static const
#define FE_INLINE

typedef min16float fehalf;
typedef min16float2 fehalf2;
typedef min16float3 fehalf3;
typedef min16float4 fehalf4;

#define FE_NUM_THREADS(x, y, z) [numthreads(x, y, z)]
#define FE_OUTPUT_TOPOLOGY(topology) [outputtopology(topology)]

FE_CONSTEXPR uint32_t kInvalidIndex = 0xffffffff;


namespace Core
{
    // clang-format off
    struct PassGraphicsPipeline {};
    struct PassComputePipeline {};
    struct PassColorTarget {};
    struct PassDepthTarget {};
    struct PassViewport {};
    struct PassScissor {};
    struct PassBufferAccess {};
    struct PassIndirectArgs {};
    struct PassTextureAccess {};
    // clang-format on
} // namespace Core


#if !FE_PLATFORM_VULKAN
namespace Bit
{
    uint32_t FieldMask(const uint32_t offset, const uint32_t size)
    {
        return ((1u << size) - 1u) << offset;
    }


    uint32_t FieldExtract(const uint32_t source, const uint32_t offset, const uint32_t size)
    {
        return (source >> offset) & ((1u << size) - 1u);
    }


    uint32_t FieldInsert(const uint32_t source, const uint32_t insert, const uint32_t offset, const uint32_t size)
    {
        const uint32_t mask = ((1u << size) - 1u);
        return (source & ~(mask << offset)) | ((insert & mask) << offset);
    }
} // namespace Bit
#endif
