--!strict

local Material = require("@engine/material")
local Pipeline = require("@engine/pipeline")

local material = Material.new("BunnyVariants")
local palette = material:specInt("Palette", { 0, 1 }, 0)

material:technique("DepthOnly", function(t, ctx)
    t.renderTargetFormats = {}
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.depthTargetFormat = "D32_SFLOAT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    return true
end)

material:technique("DepthVisibility", function(t, ctx)
    t.renderTargetFormats = { "R32G32_UINT" }
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.depthTargetFormat = "D32_SFLOAT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/Visibility/DepthVisibility.ms.hlsl"
    t.pixelShader = "Shaders/Passes/Visibility/DepthVisibility.ps.hlsl"
    return true
end)

material:technique("Opaque", function(t, ctx)
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfEqual()
    t.depthTargetFormat = "D32_SFLOAT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.renderTargetFormats = { "B10G11R11_UFLOAT" }
    t.pixelShader = "Shaders/Materials/BunnyVariants.ps.hlsl"
    t.shaderDefines["BUNNY_PALETTE"] = tostring(ctx:getInt(palette))
    return true
end)

material:technique("SurfaceEvaluate", function(t, ctx)
    t.computeShader = "Shaders/Materials/EvaluateBunnyVariants.cs.hlsl"
    t.shaderDefines["BUNNY_PALETTE"] = tostring(ctx:getInt(palette))
    return true
end)

material:technique("DeferredShade", function(t, ctx)
    t.computeShader = "Shaders/Deferred/ShadeSimpleLit.cs.hlsl"
    return true
end)

return material
