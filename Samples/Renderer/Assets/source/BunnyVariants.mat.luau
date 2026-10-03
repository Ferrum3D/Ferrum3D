--!strict

local Material = require("@engine/material")
local Pipeline = require("@engine/pipeline")

local material = Material.new("BunnyVariants")
local palette = material:specInt("Palette", { 0, 1, 2, 3 }, 0)

material:technique("DepthOnly", function(t, ctx)
    t.renderTargetFormats = {}
    if ctx:getInt(palette) == 2 then
        return false
    end
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.depthTargetFormat = "D32_SFLOAT_S8_UINT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    return true
end)

material:technique("Opaque", function(t, ctx)
    if ctx:getInt(palette) == 3 then
        return false
    end
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = if ctx:getInt(palette) == 2 then Pipeline.DepthStencilState.writeIfGreater() else Pipeline.DepthStencilState.writeIfEqual()
    t.depthTargetFormat = "D32_SFLOAT_S8_UINT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.renderTargetFormats = { "B10G11R11_UFLOAT" }
    t.pixelShader = "Shaders/Materials/BunnyVariants.ps.hlsl"
    t.shaderDefines["BUNNY_PALETTE"] = tostring(ctx:getInt(palette))
    return true
end)

return material
