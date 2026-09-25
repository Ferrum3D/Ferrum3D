#pragma once

// Keep the order in sync with Vulkan/DescriptorManager.cpp.
SamplerState GPointWrapSampler : register(s2, space0);
SamplerState GPointMirrorSampler : register(s3, space0);
SamplerState GPointClampSampler : register(s4, space0);
SamplerState GPointBorderTransparentBlackSampler : register(s5, space0);

SamplerState GLinearWrapSampler : register(s6, space0);
SamplerState GLinearMirrorSampler : register(s7, space0);
SamplerState GLinearClampSampler : register(s8, space0);
SamplerState GLinearBorderTransparentBlackSampler : register(s9, space0);
