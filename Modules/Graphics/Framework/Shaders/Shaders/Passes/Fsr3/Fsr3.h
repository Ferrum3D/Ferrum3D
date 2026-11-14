#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Fsr3)
    struct Parameters final
    {
        int2 m_renderSize;
        int2 m_previousFrameRenderSize;
        int2 m_upscaleSize;
        int2 m_previousFrameUpscaleSize;
        int2 m_maxRenderSize;
        int2 m_maxUpscaleSize;
        float4 m_deviceToViewDepth;
        float2 m_jitter;
        float2 m_previousFrameJitter;
        float2 m_motionVectorScale;
        float2 m_downscaleFactor;
        float2 m_motionVectorJitterCancellation;
        float m_tanHalfFOV;
        float m_jitterSequenceLength;
        float m_deltaTime;
        float m_deltaPreExposure;
        float m_viewSpaceToMetersFactor;
        float m_frameIndex;
        float m_velocityFactor;
        float m_reactivenessScale;
        float m_shadingChangeScale;
        float m_accumulationAddedPerFrame;
        float m_minDisocclusionAccumulation;
        uint32_t m_mipCount;
        uint32_t m_numWorkGroups;
        FE_RTTI_Reflect("224C625B-101A-4B01-AEB3-4E0EF355A4DF");
    };


    struct PrepareInputsConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        Texture2DDescriptor<float> m_rInputDepth;
        Texture2DDescriptor<float4> m_rInputColorJittered;
        Texture2DDescriptor<float4> m_rInputMotionVectors;
        RWTexture2DDescriptor<float> m_rwFarthestDepth;
        RWTexture2DDescriptor<float> m_rwCurrentLuma;
        RWTexture2DDescriptor<uint32_t> m_rwReconstructedPreviousNearestDepth;
        RWTexture2DDescriptor<float> m_rwDilatedDepth;
        RWTexture2DDescriptor<float2> m_rwDilatedMotionVectors;
        FE_RTTI_Reflect("D6B8695F-A6A7-4742-8317-8CBF29BA6C10");
    };


    struct LumaPyramidConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        Texture2DDescriptor<float> m_rFarthestDepth;
        RWTexture2DDescriptor<float> m_rwFarthestDepthMip1;
        Texture2DDescriptor<float> m_rCurrentLuma;
        RWTexture2DDescriptor<float4> m_rwFrameInfo;
        RWTexture2DDescriptor<float2> m_rwSpdMip0;
        RWTexture2DDescriptor<float2> m_rwSpdMip1;
        RWTexture2DDescriptor<float2> m_rwSpdMip2;
        RWTexture2DDescriptor<float2> m_rwSpdMip3;
        RWTexture2DDescriptor<float2> m_rwSpdMip4;
        GloballyCoherentRWTexture2DDescriptor<float2> m_rwSpdMip5;
        GloballyCoherentRWTexture2DDescriptor<uint32_t> m_rwSpdGlobalAtomic;
        FE_RTTI_Reflect("704091C5-A02E-412A-BBD9-3D37C9BABAB0");
    };


    struct ShadingChangePyramidConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        Texture2DDescriptor<float> m_rCurrentLuma;
        Texture2DDescriptor<float> m_rPreviousLuma;
        Texture2DDescriptor<float2> m_rDilatedMotionVectors;
        RWTexture2DDescriptor<float2> m_rwSpdMip0;
        RWTexture2DDescriptor<float2> m_rwSpdMip1;
        RWTexture2DDescriptor<float2> m_rwSpdMip2;
        RWTexture2DDescriptor<float2> m_rwSpdMip3;
        RWTexture2DDescriptor<float2> m_rwSpdMip4;
        GloballyCoherentRWTexture2DDescriptor<float2> m_rwSpdMip5;
        GloballyCoherentRWTexture2DDescriptor<uint32_t> m_rwSpdGlobalAtomic;
        FE_RTTI_Reflect("A082286F-A31A-4CBC-9CD5-A748BFD0EC2F");
    };


    struct ShadingChangeConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        Texture2DDescriptor<float2> m_rSpdMips;
        RWTexture2DDescriptor<float> m_rwShadingChange;
        FE_RTTI_Reflect("4C57CFA3-9BC6-42E8-B51B-9FF6F90216CA");
    };


    struct PrepareReactivityConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        Texture2DDescriptor<float> m_rReactiveMask;
        Texture2DDescriptor<float> m_rTransparencyAndCompositionMask;
        Texture2DDescriptor<float> m_rAccumulation;
        RWTexture2DDescriptor<float> m_rwAccumulation;
        Texture2DDescriptor<float> m_rShadingChange;
        Texture2DDescriptor<float> m_rCurrentLuma;
        RWTexture2DDescriptor<float> m_rwNewLocks;
        Texture2DDescriptor<uint32_t> m_rReconstructedPreviousNearestDepth;
        Texture2DDescriptor<float2> m_rDilatedMotionVectors;
        Texture2DDescriptor<float> m_rDilatedDepth;
        RWTexture2DDescriptor<float4> m_rwDilatedReactiveMasks;
        FE_RTTI_Reflect("3B569F59-2651-44E0-AB76-5207C76C84A5");
    };


    struct LumaInstabilityConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        RWTexture2DDescriptor<float4> m_rwLumaHistory;
        Texture2DDescriptor<float4> m_rLumaHistory;
        Texture2DDescriptor<float> m_rFarthestDepthMip1;
        Texture2DDescriptor<float> m_rCurrentLuma;
        RWTexture2DDescriptor<float> m_rwLumaInstability;
        Texture2DDescriptor<float2> m_rDilatedMotionVectors;
        Texture2DDescriptor<float4> m_rDilatedReactiveMasks;
        Texture2DDescriptor<float4> m_rFrameInfo;
        FE_RTTI_Reflect("9E952B7D-5A84-4291-B3F8-64F6A1FFB015");
    };


    struct AccumulateConstants final
    {
        ConstantsPtr<Parameters> m_parameters;
        SamplerDescriptor m_linearClamp;
        Texture2DDescriptor<float4> m_rInputColorJittered;
        Texture2DDescriptor<float4> m_rInternalUpscaledColor;
        RWTexture2DDescriptor<float4> m_rwInternalUpscaledColor;
        RWTexture2DDescriptor<float4> m_rwUpscaledOutput;
        Texture2DDescriptor<float> m_rFarthestDepthMip1;
        Texture2DDescriptor<float> m_rCurrentLuma;
        Texture2DDescriptor<float> m_rLumaInstability;
        RWTexture2DDescriptor<float> m_rwNewLocks;
        Texture2DDescriptor<float2> m_rDilatedMotionVectors;
        Texture2DDescriptor<float4> m_rDilatedReactiveMasks;
        FE_RTTI_Reflect("51F4D994-AA75-4C7D-9ABE-DA10AEA87741");
    };


    struct ClearConstants final
    {
        RWTexture2DDescriptor<float4> m_target;
        uint2 m_size;
        float4 m_value;
        FE_RTTI_Reflect("254AD2CB-623F-4425-947D-172CC3C25A6B");
    };


    struct ClearUintConstants final
    {
        RWTexture2DDescriptor<uint32_t> m_target;
        uint2 m_size;
        FE_RTTI_Reflect("E00BF0B4-AA4C-4F4F-9C2D-8DD70BC3844D");
    };
FE_HOST_END_NAMESPACE
