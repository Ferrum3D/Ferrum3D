--!strict
local Material = require("@engine/material")
local Pipeline = require("@engine/pipeline")
local material = Material.new("TransparentPBR")
material:parameter("BaseColor", "Vector", "Material", { 0.15, 0.6, 0.85, 0.35 })
material:parameter("Properties", "Vector", "Material", { 0.22, 0.0, 0.0, 0.0 })
material:technique("ForwardTransparency", function(t, ctx)
    t.renderTargetFormats = { "R16G16B16A16_SFLOAT", "R16_SFLOAT" }
    t.depthTargetFormat = "D32_SFLOAT"
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.depthStencil.depthWriteEnabled = false
    t.meshShader = "Shaders/Materials/ForwardTransparent.ms.hlsl"
    t.pixelShader = "Shaders/Materials/ForwardTransparent.ps.hlsl"
    local accumulation = Pipeline.BlendState.disabled()
    accumulation.blendEnabled = true
    accumulation.destinationFactor = "One"
    accumulation.destinationAlphaFactor = "One"
    local revealage = Pipeline.BlendState.disabled()
    revealage.blendEnabled = true
    revealage.sourceFactor = "Zero"
    revealage.destinationFactor = "OneMinusSrcColor"
    t.targetBlends = { accumulation, revealage }
    return true
end)
-- Requests the sun shadow pages that the forward pass samples, since transparent surfaces are absent from the depth.
material:technique("ShadowPageMark", function(t, ctx)
    t.depthTargetFormat = "D32_SFLOAT"
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.depthStencil.depthWriteEnabled = false
    t.meshShader = "Shaders/Shadows/MarkTransparentPages.ms.hlsl"
    t.pixelShader = "Shaders/Shadows/MarkTransparentPages.ps.hlsl"
    return true
end)

return material
