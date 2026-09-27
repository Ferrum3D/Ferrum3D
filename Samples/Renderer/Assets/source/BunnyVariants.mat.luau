--!strict

local Material = require("@engine/material")
local Pipeline = require("@engine/pipeline")

local material = Material.new("BunnyVariants")
local palette = material:specInt("Palette", { 0, 1 }, 0)

material:technique("DepthOnly", function(t, ctx)
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    return true
end)

material:technique("Opaque", function(t, ctx)
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.depthStencil = Pipeline.DepthStencilState.writeIfEqual()
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.pixelShader = "Shaders/Materials/BunnyVariants.ps.hlsl"
    t.shaderDefines["BUNNY_PALETTE"] = tostring(ctx:getInt(palette))
    return true
end)

return material
