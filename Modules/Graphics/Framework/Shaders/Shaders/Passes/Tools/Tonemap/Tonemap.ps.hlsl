#include <Shaders/Base/Math.hlsli>
#include <Shaders/Passes/Tools/Blit/Blit.hlsli>

/*
 * AgX adapted from three.js (https://github.com/mrdoob/three.js), based on Filament and Blender.
 * The MIT License
 * Copyright (c) 2010-2026 three.js authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
float3 TonemapAgx(const float3 hdr)
{
    // clang-format off
    const float3x3 srgbToRec2020 = float3x3(0.6274f, 0.3293f, 0.0433f,
                                            0.0691f, 0.9195f, 0.0113f,
                                            0.0164f, 0.0880f, 0.8956f);
    const float3x3 inset = float3x3(0.8566271533f, 0.0951212405f, 0.0482516061f,
                                   0.1373189729f, 0.7612419906f, 0.1014390365f,
                                   0.1118982130f, 0.0767994186f, 0.8113023684f);
    const float3x3 outset = float3x3(1.1271005818f, -0.1106066431f, -0.0164939387f,
                                    -0.1413297635f, 1.1578237022f, -0.0164939387f,
                                    -0.1413297635f, -0.1106066431f, 1.2519364066f);
    const float3x3 rec2020ToSrgb = float3x3(1.6605f, -0.5876f, -0.0728f,
                                           -0.1246f, 1.1329f, -0.0083f,
                                           -0.0182f, -0.1006f, 1.1187f);
    // clang-format on
    const float minEV = -12.47393f;
    const float maxEV = 4.026069f;

    // Expand the gamut before compressing 16.5 stops around middle gray (0.18).
    const float3 wideColor = mul(srgbToRec2020, max(hdr, 0.0f));
    const float3 insetColor = mul(inset, wideColor);
    const float3 logColor = log2(max(insetColor, 1.0e-10f));
    const float3 x = saturate((logColor - minEV) / (maxEV - minEV));
    const float3 x2 = x * x;
    const float3 x4 = x2 * x2;
    const float3 contrast =
        15.5f * x4 * x2 - 40.14f * x4 * x + 31.96f * x4 - 6.868f * x2 * x + 0.4298f * x2 + 0.1191f * x - 0.00232f;

    // Decode AgX's gamma 2.2 output before converting Rec.2020 primaries to sRGB.
    const float3 outsetColor = mul(outset, contrast);
    const float3 linearRec2020 = pow(max(outsetColor, 0.0f), 2.2f);
    const float3 linearSrgb = mul(rec2020ToSrgb, linearRec2020);
    return saturate(linearSrgb);
}


void main(const in PixelAttributes input, out float4 color : SV_Target0)
{
    const float3 hdr = GConstants.m_input.Sample(GConstants.m_sampler.Get(), input.m_texCoord).rgb;
    const float3 mapped = TonemapAgx(hdr);
    color = float4(Math::LinearToSRGB(mapped), 1.0f);
}
