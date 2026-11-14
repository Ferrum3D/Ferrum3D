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

#include <Shaders/Passes/Fsr3/Configuration.hlsli>
#include <Shaders/Passes/Fsr3/Fsr3.h>

[[vk::push_constant]] LumaInstabilityConstants GConstants;
static Parameters GParameters;

#define FSR3UPSCALER_BIND_SRV_DILATED_REACTIVE_MASKS 1
#define FSR3UPSCALER_BIND_SRV_DILATED_MOTION_VECTORS 2
#define FSR3UPSCALER_BIND_SRV_FRAME_INFO 3
#define FSR3UPSCALER_BIND_SRV_LUMA_HISTORY 4
#define FSR3UPSCALER_BIND_SRV_FARTHEST_DEPTH_MIP1 5
#define FSR3UPSCALER_BIND_SRV_CURRENT_LUMA 6
#define FSR3UPSCALER_BIND_UAV_LUMA_HISTORY 0
#define FSR3UPSCALER_BIND_UAV_LUMA_INSTABILITY 1
#define FSR3UPSCALER_BIND_CB_FSR3UPSCALER 0

// clang-format off
#include <Shaders/Passes/Fsr3/Callbacks.hlsli>
#include "fsr3upscaler/ffx_fsr3upscaler_common.h"
#include "fsr3upscaler/ffx_fsr3upscaler_luma_instability.h"
// clang-format on

FE_NUM_THREADS(8, 8, 1)
void main(int2 iDispatchThreadId : SV_DispatchThreadID)
{
    GParameters = GConstants.m_parameters.Load();
    if (any(iDispatchThreadId >= RenderSize()))
        return;

    LumaInstability(iDispatchThreadId);
}
