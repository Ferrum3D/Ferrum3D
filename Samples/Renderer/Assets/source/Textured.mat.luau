--!strict

local Material = require("@engine/material")
local Pipeline = require("@engine/pipeline")

local material = Material.new("Textured")
local twoSided = material:specBool("TwoSided", false)

material:parameter("Tint", "Vector", "Material", { 1, 1, 1, 1 })
material:parameter("Albedo", "Texture2D", "Material")
material:parameter("UvScale", "Vector", "Instance", { 1, 1, 0, 0 })

local function setCull(t, ctx)
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    if ctx:getBool(twoSided) then
        t.rasterization.cullMode = "None"
    end
end

material:technique("DepthOnly", function(t, ctx)
    setCull(t, ctx)
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    return true
end)

material:technique("Opaque", function(t, ctx)
    setCull(t, ctx)
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.pixelShader = "Shaders/Materials/Textured.ps.hlsl"
    t.depthStencil = Pipeline.DepthStencilState.writeIfEqual()
    return true
end)

return material
