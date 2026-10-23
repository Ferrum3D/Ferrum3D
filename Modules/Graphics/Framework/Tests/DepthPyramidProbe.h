#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::DepthPyramidProbe)

    FE_CONSTEXPR uint32_t kFillMode = 0;
    FE_CONSTEXPR uint32_t kReadMode = 1;
    FE_CONSTEXPR uint32_t kThreadGroupSize = 8;

    //! Fills m_target with a hashed depth pattern, or copies one mip of m_source into m_output.
    struct Constants final
    {
        Texture2DDescriptor<float> m_source;
        RWTexture2DDescriptor<float> m_target;
        RWStructuredBufferDescriptor<float> m_output;
        uint32_t m_mode;
        uint2 m_size;
        uint32_t m_outputOffset;
        uint32_t m_padding;

        FE_RTTI_Reflect("C872C975-3141-45B2-A79B-0F5CD52F8892");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("850F693D-EAB2-4A53-B731-962E19C45D1F");
    };


    FE_INLINE float HashDepth(const uint32_t x, const uint32_t y)
    {
        uint32_t hash = (x * 73856093u) ^ (y * 19349663u);
        hash ^= hash >> 13;
        hash *= 0x5bd1e995u;
        hash ^= hash >> 15;
        // A power-of-two scale keeps the values exact on GPUs, whose division is not correctly rounded.
        return float(hash & 0xffffu) * (1.0f / 65536.0f);
    }

#if FE_HOST
    void Check(Core::Device * device,
               Core::ResourcePool * pool,
               Core::GraphicsQueue * queue,
               Core::DescriptorManager * descriptors,
               Core::PipelineFactory * pipelines);
#endif

FE_HOST_END_NAMESPACE
