#pragma once

#define FE_HOST_BEGIN_NAMESPACE(name)
#define FE_HOST_END_NAMESPACE

#if defined(__JETBRAINS_IDE__) || defined(__INTELLISENSE__)
typedef uint uint32_t;
#    define fe_globallycoherent

#    define fe_mesh_payload
#    define fe_mesh_vertices
#    define fe_mesh_indices

#    define FE_CONST const
#else
#    define fe_globallycoherent globallycoherent

#    define fe_mesh_payload payload
#    define fe_mesh_vertices vertices
#    define fe_mesh_indices indices

#    define FE_CONST
#endif

typedef min16float fehalf;
typedef min16float2 fehalf2;
typedef min16float3 fehalf3;
typedef min16float4 fehalf4;

#define FE_NUM_THREADS(x, y, z) [numthreads(x, y, z)]
#define FE_OUTPUT_TOPOLOGY(topology) [outputtopology(topology)]

static const uint32_t kInvalidIndex = 0xffffffff;
static const uint kLanesPerWave = 32;


namespace Bit
{
    uint32_t FieldExtract(const uint32_t source, const uint32_t offset, const uint32_t size)
    {
        // Gets optimized to v_bfe_u32.
        return (source >> offset) & ((1u << size) - 1u);
    }


    uint32_t FieldInsert(const uint32_t mask, const uint32_t a, const uint32_t b)
    {
        return (a & mask) | (b & ~mask);
    }


    uint32_t FieldMask(const uint32_t width, const uint32_t offset)
    {
        return ((1u << width) - 1u) << offset;
    }
} // namespace Bit


namespace Math
{
    namespace Pack
    {
        //
        // 16-bit Float
        //


        uint2 RGBA32FloatToRGBA16Float(const float4 source)
        {
            return uint2((f32tof16(source.x) & 0xffff) | (f32tof16(source.y) << 16),
                         (f32tof16(source.z) & 0xffff) | (f32tof16(source.w) << 16));
        }


        uint2 RGBA32FloatToRGBA16Float(const fehalf4 source)
        {
            // Things like this should in theory be optimized by drivers to something like a bit-cast.
            return uint2((f32tof16(source.x) & 0xffff) | (f32tof16(source.y) << 16),
                         (f32tof16(source.z) & 0xffff) | (f32tof16(source.w) << 16));
        }


        float4 RGBA16FloatToRGBA32Float(const uint2 source)
        {
            return float4(f16tof32(source.x), f16tof32(source.x >> 16), f16tof32(source.y), f16tof32(source.y >> 16));
        }


        fehalf4 RGBA16FloatToRGBA32Float_Half(const uint2 source)
        {
            return fehalf4(f16tof32(source.x), f16tof32(source.x >> 16), f16tof32(source.y), f16tof32(source.y >> 16));
        }


        uint RG32FloatToRG16Float(const float2 source)
        {
            return (f32tof16(source.x) & 0xffff) | (f32tof16(source.y) << 16);
        }


        float2 RG16FloatToRG32Float(const uint source)
        {
            return float2(f16tof32(source), f16tof32(source >> 16));
        }


        //
        // 8-bit Unorm
        //


        float4 RGBA8UnormToRGBA32Float(const uint32_t source)
        {
            return float4(((source >> 0) & 0xff), ((source >> 8) & 0xff), ((source >> 16) & 0xff), ((source >> 24) & 0xff))
                / 255.0f;
        }


        uint32_t RGBA32FloatToRGBA8Unorm(const float4 source)
        {
            const float4 t = source * 255.0f;
            return uint32_t(t.x) | (uint32_t(t.y) << 8) | (uint32_t(t.z) << 16) | (uint32_t(t.w) << 24);
        }


        //
        // A2B10G10R10 Unorm
        //


        uint32_t RGBA32FloatToA2R10G10B10Unorm(const float4 source)
        {
            const float4 t = source * float4(1023.0f, 1023.0f, 1023.0f, 3.0f);
            return (uint32_t(t.x) & 0x3ff) | ((uint32_t(t.y) << 10) & 0x3ff) | ((uint32_t(t.z) << 20) & 0x3ff)
                | (uint32_t(t.w) << 30);
        }


        float4 A2R10G10B10UnormToRGBA32Float(const uint32_t source)
        {
            const float4 t = float4(source & 0x3ff, (source >> 10) & 0x3ff, (source >> 20) & 0x3ff, source >> 30);
            return t / float4(1023.0f, 1023.0f, 1023.0f, 3.0f);
        }
    } // namespace Pack
} // namespace Math
