// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Engine descriptor adapter for the vendored FSR3 upscaler callbacks.
#pragma once
#include "ffx_core.h"
#include "fsr3upscaler/ffx_fsr3upscaler_resources.h"
#include <Shaders/Shading/Exposure.h>

#define s_LinearClamp GConstants.m_linearClamp.Get()

FfxInt32x2 RenderSize()
{
    return GParameters.m_renderSize;
}


FfxInt32x2 PreviousFrameRenderSize()
{
    return GParameters.m_previousFrameRenderSize;
}


FfxInt32x2 MaxRenderSize()
{
    return GParameters.m_maxRenderSize;
}


FfxInt32x2 UpscaleSize()
{
    return GParameters.m_upscaleSize;
}


FfxInt32x2 PreviousFrameUpscaleSize()
{
    return GParameters.m_previousFrameUpscaleSize;
}


FfxInt32x2 MaxUpscaleSize()
{
    return GParameters.m_maxUpscaleSize;
}


FfxFloat32x2 Jitter()
{
    return GParameters.m_jitter;
}


FfxFloat32x2 PreviousFrameJitter()
{
    return GParameters.m_previousFrameJitter;
}


FfxFloat32x4 DeviceToViewSpaceTransformFactors()
{
    return GParameters.m_deviceToViewDepth;
}


FfxFloat32x2 MotionVectorScale()
{
    return GParameters.m_motionVectorScale;
}


FfxFloat32x2 DownscaleFactor()
{
    return GParameters.m_downscaleFactor;
}


FfxFloat32x2 MotionVectorJitterCancellation()
{
    return GParameters.m_motionVectorJitterCancellation;
}


FfxFloat32 TanHalfFoV()
{
    return GParameters.m_tanHalfFOV;
}


FfxFloat32 JitterSequenceLength()
{
    return GParameters.m_jitterSequenceLength;
}


FfxFloat32 DeltaTime()
{
    return GParameters.m_deltaTime;
}


FfxFloat32 DeltaPreExposure()
{
    return GParameters.m_deltaPreExposure;
}


FfxFloat32 ViewSpaceToMetersFactor()
{
    return GParameters.m_viewSpaceToMetersFactor;
}


FfxFloat32 FrameIndex()
{
    return GParameters.m_frameIndex;
}


FfxFloat32 VelocityFactor()
{
    return GParameters.m_velocityFactor;
}


FfxFloat32 AccumulationAddedPerFrame()
{
    return GParameters.m_accumulationAddedPerFrame;
}


FfxFloat32 MinDisocclusionAccumulation()
{
    return GParameters.m_minDisocclusionAccumulation;
}


uint MipCount()
{
    return GParameters.m_mipCount;
}


uint NumWorkGroups()
{
    return GParameters.m_numWorkGroups;
}


uint2 WorkGroupOffset()
{
    return 0;
}


uint2 SPD_RenderSize()
{
    return RenderSize();
}

#if defined(FSR3UPSCALER_BIND_SRV_SPD_MIPS)
#    define r_spd_mips GConstants.m_rSpdMips.Get()

FfxInt32x2 GetSPDMipDimensions(FfxUInt32 uMipLevel)
{
    FfxUInt32 uWidth;
    FfxUInt32 uHeight;
    FfxUInt32 uLevels;
    r_spd_mips.GetDimensions(uMipLevel, uWidth, uHeight, uLevels);

    return FfxInt32x2(uWidth, uHeight);
}


FfxFloat32x2 SampleSPDMipLevel(FfxFloat32x2 fUV, FfxUInt32 mipLevel)
{
    return r_spd_mips.SampleLevel(s_LinearClamp, fUV, mipLevel);
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_INPUT_DEPTH)
#    define r_input_depth GConstants.m_rInputDepth.Get()

FfxFloat32 LoadInputDepth(FfxUInt32x2 iPxPos)
{
    return r_input_depth[iPxPos];
}


FfxFloat32 SampleInputDepth(FfxFloat32x2 fUV)
{
    return r_input_depth.SampleLevel(s_LinearClamp, fUV, 0).x;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_REACTIVE_MASK)
#    define r_reactive_mask GConstants.m_rReactiveMask.Get()

FfxFloat32 LoadReactiveMask(FfxUInt32x2 iPxPos)
{
    FfxUInt32 width;
    FfxUInt32 height;
    r_reactive_mask.GetDimensions(width, height);
    return r_reactive_mask[min(iPxPos, FfxUInt32x2(width - 1, height - 1))] * GParameters.m_reactivenessScale;
}


FfxInt32x2 GetReactiveMaskResourceDimensions()
{
    FfxUInt32 uWidth;
    FfxUInt32 uHeight;
    r_reactive_mask.GetDimensions(uWidth, uHeight);

    return FfxInt32x2(uWidth, uHeight);
}


FfxFloat32 SampleReactiveMask(FfxFloat32x2 fUV)
{
    return r_reactive_mask.SampleLevel(s_LinearClamp, fUV, 0).x * GParameters.m_reactivenessScale;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK)
#    define r_transparency_and_composition_mask GConstants.m_rTransparencyAndCompositionMask.Get()

FfxFloat32 LoadTransparencyAndCompositionMask(FfxUInt32x2 iPxPos)
{
    return r_transparency_and_composition_mask[iPxPos];
}


FfxInt32x2 GetTransparencyAndCompositionMaskResourceDimensions()
{
    FfxUInt32 uWidth;
    FfxUInt32 uHeight;
    r_transparency_and_composition_mask.GetDimensions(uWidth, uHeight);

    return FfxInt32x2(uWidth, uHeight);
}


FfxFloat32 SampleTransparencyAndCompositionMask(FfxFloat32x2 fUV)
{
    return r_transparency_and_composition_mask.SampleLevel(s_LinearClamp, fUV, 0).x;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_INPUT_COLOR)
#    define r_input_color_jittered GConstants.m_rInputColorJittered.Get()

FfxFloat32x3 LoadInputColor(FfxUInt32x2 iPxPos)
{
    return r_input_color_jittered[iPxPos].rgb;
}


FfxFloat32x3 SampleInputColor(FfxFloat32x2 fUV)
{
    return r_input_color_jittered.SampleLevel(s_LinearClamp, fUV, 0).rgb;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_INPUT_MOTION_VECTORS)
#    define r_input_motion_vectors GConstants.m_rInputMotionVectors.Get()

FfxFloat32x2 LoadInputMotionVector(FfxUInt32x2 iPxDilatedMotionVectorPos)
{
    FfxFloat32x2 fSrcMotionVector = r_input_motion_vectors[iPxDilatedMotionVectorPos].xy;

    FfxFloat32x2 fUvMotionVector = fSrcMotionVector * MotionVectorScale();

#    if FFX_FSR3UPSCALER_OPTION_JITTERED_MOTION_VECTORS
    fUvMotionVector -= MotionVectorJitterCancellation();
#    endif

    return fUvMotionVector;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_INTERNAL_UPSCALED)
#    define r_internal_upscaled_color GConstants.m_rInternalUpscaledColor.Get()

FfxFloat32x4 LoadHistory(FfxUInt32x2 iPxHistory)
{
    return r_internal_upscaled_color[iPxHistory];
}


FfxFloat32x4 SampleHistory(FfxFloat32x2 fUV)
{
    return r_internal_upscaled_color.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_LUMA_HISTORY)
#    define rw_luma_history GConstants.m_rwLumaHistory.Get()

void StoreLumaHistory(FfxUInt32x2 iPxPos, FfxFloat32x4 fLumaHistory)
{
    rw_luma_history[iPxPos] = fLumaHistory;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_LUMA_HISTORY)
#    define r_luma_history GConstants.m_rLumaHistory.Get()

FfxFloat32x4 LoadLumaHistory(FfxInt32x2 iPxPos)
{
    return r_luma_history[iPxPos];
}


FfxFloat32x4 SampleLumaHistory(FfxFloat32x2 fUV)
{
    return r_luma_history.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif


#if defined(FSR3UPSCALER_BIND_UAV_INTERNAL_UPSCALED)
#    define rw_internal_upscaled_color GConstants.m_rwInternalUpscaledColor.Get()

void StoreReprojectedHistory(FfxUInt32x2 iPxHistory, FfxFloat32x4 fHistory)
{
    rw_internal_upscaled_color[iPxHistory] = fHistory;
}


void StoreInternalColorAndWeight(FfxUInt32x2 iPxPos, FfxFloat32x4 fColorAndWeight)
{
    rw_internal_upscaled_color[iPxPos] = fColorAndWeight;
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_UPSCALED_OUTPUT)
#    define rw_upscaled_output GConstants.m_rwUpscaledOutput.Get()

void StoreUpscaledOutput(FfxUInt32x2 iPxPos, FfxFloat32x3 fColor)
{
    rw_upscaled_output[iPxPos] = FfxFloat32x4(fColor, 1.f);
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_ACCUMULATION)
#    define r_accumulation GConstants.m_rAccumulation.Get()

FfxFloat32 SampleAccumulation(FfxFloat32x2 fUV)
{
    return r_accumulation.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_ACCUMULATION)
#    define rw_accumulation GConstants.m_rwAccumulation.Get()

void StoreAccumulation(FfxUInt32x2 iPxPos, FfxFloat32 fAccumulation)
{
    rw_accumulation[iPxPos] = fAccumulation;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_SHADING_CHANGE)
#    define r_shading_change GConstants.m_rShadingChange.Get()

FfxFloat32 LoadShadingChange(FfxUInt32x2 iPxPos)
{
    return r_shading_change[iPxPos] * GParameters.m_shadingChangeScale;
}


FfxFloat32 SampleShadingChange(FfxFloat32x2 fUV)
{
    return r_shading_change.SampleLevel(s_LinearClamp, fUV, 0) * GParameters.m_shadingChangeScale;
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_SHADING_CHANGE)
#    define rw_shading_change GConstants.m_rwShadingChange.Get()

void StoreShadingChange(FfxUInt32x2 iPxPos, FfxFloat32 fShadingChange)
{
    rw_shading_change[iPxPos] = fShadingChange;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_FARTHEST_DEPTH)
#    define r_farthest_depth GConstants.m_rFarthestDepth.Get()

FfxInt32x2 GetFarthestDepthResourceDimensions()
{
    FfxUInt32 uWidth;
    FfxUInt32 uHeight;
    r_farthest_depth.GetDimensions(uWidth, uHeight);

    return FfxInt32x2(uWidth, uHeight);
}


FfxFloat32 LoadFarthestDepth(FfxUInt32x2 iPxPos)
{
    return r_farthest_depth[iPxPos];
}


FfxFloat32 SampleFarthestDepth(FfxFloat32x2 fUV)
{
    return r_farthest_depth.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_FARTHEST_DEPTH)
#    define rw_farthest_depth GConstants.m_rwFarthestDepth.Get()

void StoreFarthestDepth(FfxUInt32x2 iPxPos, FfxFloat32 fDepth)
{
    rw_farthest_depth[iPxPos] = fDepth;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_FARTHEST_DEPTH_MIP1)
#    define r_farthest_depth_mip1 GConstants.m_rFarthestDepthMip1.Get()

FfxInt32x2 GetFarthestDepthMip1ResourceDimensions()
{
    FfxUInt32 uWidth;
    FfxUInt32 uHeight;
    r_farthest_depth_mip1.GetDimensions(uWidth, uHeight);

    return FfxInt32x2(uWidth, uHeight);
}


FfxFloat32 LoadFarthestDepthMip1(FfxUInt32x2 iPxPos)
{
    return r_farthest_depth_mip1[iPxPos];
}


FfxFloat32 SampleFarthestDepthMip1(FfxFloat32x2 fUV)
{
    return r_farthest_depth_mip1.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_FARTHEST_DEPTH_MIP1)
#    define rw_farthest_depth_mip1 GConstants.m_rwFarthestDepthMip1.Get()

void StoreFarthestDepthMip1(FfxUInt32x2 iPxPos, FfxFloat32 fDepth)
{
    rw_farthest_depth_mip1[iPxPos] = fDepth;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_CURRENT_LUMA)
#    define r_current_luma GConstants.m_rCurrentLuma.Get()

FfxFloat32 LoadCurrentLuma(FfxUInt32x2 iPxPos)
{
    return r_current_luma[iPxPos];
}


FfxFloat32 SampleCurrentLuma(FfxFloat32x2 uv)
{
    return r_current_luma.SampleLevel(s_LinearClamp, uv, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_CURRENT_LUMA)
#    define rw_current_luma GConstants.m_rwCurrentLuma.Get()

void StoreCurrentLuma(FfxUInt32x2 iPxPos, FfxFloat32 fLuma)
{
    rw_current_luma[iPxPos] = fLuma;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_LUMA_INSTABILITY)
#    define r_luma_instability GConstants.m_rLumaInstability.Get()

FfxFloat32 SampleLumaInstability(FfxFloat32x2 uv)
{
    return r_luma_instability.SampleLevel(s_LinearClamp, uv, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_LUMA_INSTABILITY)
#    define rw_luma_instability GConstants.m_rwLumaInstability.Get()

void StoreLumaInstability(FfxUInt32x2 iPxPos, FfxFloat32 fLumaInstability)
{
    rw_luma_instability[iPxPos] = fLumaInstability;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_PREVIOUS_LUMA)
#    define r_previous_luma GConstants.m_rPreviousLuma.Get()

FfxFloat32 LoadPreviousLuma(FfxUInt32x2 iPxPos)
{
    return r_previous_luma[iPxPos];
}


FfxFloat32 SamplePreviousLuma(FfxFloat32x2 uv)
{
    return r_previous_luma.SampleLevel(s_LinearClamp, uv, 0);
}
#endif


#if defined(FSR3UPSCALER_BIND_UAV_NEW_LOCKS)
#    define rw_new_locks GConstants.m_rwNewLocks.Get()

FfxFloat32 LoadRwNewLocks(FfxUInt32x2 iPxPos)
{
    return rw_new_locks[iPxPos];
}


void StoreNewLocks(FfxUInt32x2 iPxPos, FfxFloat32 newLock)
{
    rw_new_locks[iPxPos] = newLock;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH)
#    define r_reconstructed_previous_nearest_depth GConstants.m_rReconstructedPreviousNearestDepth.Get()

FfxFloat32 LoadReconstructedPrevDepth(FfxUInt32x2 iPxPos)
{
    return asfloat(r_reconstructed_previous_nearest_depth[iPxPos]);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH)
#    define rw_reconstructed_previous_nearest_depth GConstants.m_rwReconstructedPreviousNearestDepth.Get()

void StoreReconstructedDepth(FfxUInt32x2 iPxSample, FfxFloat32 fDepth)
{
    FfxUInt32 uDepth = asuint(fDepth);

#    if FFX_FSR3UPSCALER_OPTION_INVERTED_DEPTH
    InterlockedMax(rw_reconstructed_previous_nearest_depth[iPxSample], uDepth);
#    else
    InterlockedMin(rw_reconstructed_previous_nearest_depth[iPxSample], uDepth); // min for standard, max for inverted depth
#    endif
}


void SetReconstructedDepth(FfxUInt32x2 iPxSample, const FfxUInt32 uValue)
{
    rw_reconstructed_previous_nearest_depth[iPxSample] = uValue;
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_DILATED_DEPTH)
#    define rw_dilated_depth GConstants.m_rwDilatedDepth.Get()

void StoreDilatedDepth(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32 fDepth)
{
    rw_dilated_depth[iPxPos] = fDepth;
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_DILATED_MOTION_VECTORS)
#    define rw_dilated_motion_vectors GConstants.m_rwDilatedMotionVectors.Get()

void StoreDilatedMotionVector(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32x2 fMotionVector)
{
    rw_dilated_motion_vectors[iPxPos] = fMotionVector;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_DILATED_MOTION_VECTORS)
#    define r_dilated_motion_vectors GConstants.m_rDilatedMotionVectors.Get()

FfxFloat32x2 LoadDilatedMotionVector(FfxUInt32x2 iPxInput)
{
    return r_dilated_motion_vectors[iPxInput];
}


FfxFloat32x2 SampleDilatedMotionVector(FfxFloat32x2 fUV)
{
    return r_dilated_motion_vectors.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_DILATED_DEPTH)
#    define r_dilated_depth GConstants.m_rDilatedDepth.Get()

FfxFloat32 LoadDilatedDepth(FfxUInt32x2 iPxInput)
{
    return r_dilated_depth[iPxInput];
}


FfxFloat32 SampleDilatedDepth(FfxFloat32x2 fUV)
{
    return r_dilated_depth.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

float Exposure()
{
    return Shading::kExposure / Shading::kPreExposure;
}

// BEGIN: FSR3UPSCALER_BIND_SRV_LANCZOS_LUT
#if defined(FSR3UPSCALER_BIND_SRV_LANCZOS_LUT)
#    define r_lanczos_lut GConstants.m_rLanczosLut.Get()
#endif

FfxFloat32 SampleLanczos2Weight(FfxFloat32 x)
{
#if defined(FSR3UPSCALER_BIND_SRV_LANCZOS_LUT)
    return r_lanczos_lut.SampleLevel(s_LinearClamp, FfxFloat32x2(x / 2, 0.5f), 0);
#else
    return 0.f;
#endif
}
// END: FSR3UPSCALER_BIND_SRV_LANCZOS_LUT

#if defined(FSR3UPSCALER_BIND_SRV_DILATED_REACTIVE_MASKS)
#    define r_dilated_reactive_masks GConstants.m_rDilatedReactiveMasks.Get()

FfxFloat32x4 SampleDilatedReactiveMasks(FfxFloat32x2 fUV)
{
    return r_dilated_reactive_masks.SampleLevel(s_LinearClamp, fUV, 0);
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_DILATED_REACTIVE_MASKS)
#    define rw_dilated_reactive_masks GConstants.m_rwDilatedReactiveMasks.Get()

void StoreDilatedReactiveMasks(FFX_PARAMETER_IN FfxUInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32x4 fDilatedReactiveMasks)
{
    rw_dilated_reactive_masks[iPxPos] = fDilatedReactiveMasks;
}
#endif


#if defined(FSR3UPSCALER_BIND_UAV_AUTOREACTIVE) && defined(FSR3UPSCALER_BIND_UAV_AUTOCOMPOSITION)

#    define rw_output_autoreactive GConstants.m_rwOutputAutoreactive.Get()
#    define rw_output_autocomposition GConstants.m_rwOutputAutocomposition.Get()

void StoreAutoReactive(FFX_PARAMETER_IN FFX_MIN16_I2 iPxPos, FFX_PARAMETER_IN FFX_MIN16_F2 fReactive)
{
    rw_output_autoreactive[iPxPos] = fReactive.x;

    rw_output_autocomposition[iPxPos] = fReactive.y;
}
#endif


#if defined(FSR3UPSCALER_BIND_UAV_FRAME_INFO)
#    define rw_frame_info GConstants.m_rwFrameInfo.Get()

FfxFloat32x4 LoadFrameInfo()
{
    return rw_frame_info[FfxInt32x2(0, 0)];
}


void StoreFrameInfo(FfxFloat32x4 fInfo)
{
    rw_frame_info[FfxInt32x2(0, 0)] = fInfo;
}
#endif

#if defined(FSR3UPSCALER_BIND_SRV_FRAME_INFO)
#    define r_frame_info GConstants.m_rFrameInfo.Get()

FfxFloat32x4 FrameInfo()
{
    return r_frame_info[FfxInt32x2(0, 0)];
}
#endif

#if defined(FSR3UPSCALER_BIND_UAV_SPD_MIPS_LEVEL_0) && defined(FSR3UPSCALER_BIND_UAV_SPD_MIPS_LEVEL_1)                           \
    && defined(FSR3UPSCALER_BIND_UAV_SPD_MIPS_LEVEL_2) && defined(FSR3UPSCALER_BIND_UAV_SPD_MIPS_LEVEL_3)                        \
    && defined(FSR3UPSCALER_BIND_UAV_SPD_MIPS_LEVEL_4) && defined(FSR3UPSCALER_BIND_UAV_SPD_MIPS_LEVEL_5)

#    define rw_spd_mip0 GConstants.m_rwSpdMip0.Get()
#    define rw_spd_mip1 GConstants.m_rwSpdMip1.Get()
#    define rw_spd_mip2 GConstants.m_rwSpdMip2.Get()
#    define rw_spd_mip3 GConstants.m_rwSpdMip3.Get()
#    define rw_spd_mip4 GConstants.m_rwSpdMip4.Get()
#    define rw_spd_mip5 GConstants.m_rwSpdMip5.Get()

FfxFloat32x2 RWLoadPyramid(FFX_PARAMETER_IN FfxInt32x2 iPxPos, FFX_PARAMETER_IN FfxUInt32 index)
{
#    define LOAD(idx)                                                                                                            \
        if (index == idx)                                                                                                        \
        {                                                                                                                        \
            return rw_spd_mip##idx[iPxPos];                                                                                      \
        }
    LOAD(0);
    LOAD(1);
    LOAD(2);
    LOAD(3);
    LOAD(4);
    LOAD(5);

    return 0;

#    undef LOAD
}


void StorePyramid(FFX_PARAMETER_IN FfxInt32x2 iPxPos, FFX_PARAMETER_IN FfxFloat32x2 outValue, FFX_PARAMETER_IN FfxUInt32 index)
{
#    define STORE(idx)                                                                                                           \
        if (index == idx)                                                                                                        \
        {                                                                                                                        \
            rw_spd_mip##idx[iPxPos] = outValue;                                                                                  \
        }

    STORE(0);
    STORE(1);
    STORE(2);
    STORE(3);
    STORE(4);
    STORE(5);

#    undef STORE
}
#endif

#if defined FSR3UPSCALER_BIND_UAV_SPD_GLOBAL_ATOMIC
#    define rw_spd_global_atomic GConstants.m_rwSpdGlobalAtomic.Get()

void SPD_IncreaseAtomicCounter(inout FfxUInt32 spdCounter)
{
    InterlockedAdd(rw_spd_global_atomic[FfxInt32x2(0, 0)], 1, spdCounter);
}


void SPD_ResetAtomicCounter()
{
    rw_spd_global_atomic[FfxInt32x2(0, 0)] = 0;
}
#endif
