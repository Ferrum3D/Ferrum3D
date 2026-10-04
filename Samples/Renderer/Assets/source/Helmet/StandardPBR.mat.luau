--!strict

local Material = require("@engine/material")
local Pipeline = require("@engine/pipeline")

local material = Material.new("StandardPBR")
local twoSided = material:specBool("TwoSided", false)

material:parameter("BaseColor", "Texture2D", "Material")
material:parameter("AORoughnessMetallic", "Texture2D", "Material")
material:parameter("Normal", "Texture2D", "Material")
material:parameter("Emissive", "Texture2D", "Material")
material:parameter("UvScale", "Vector", "Instance", { 1, 1, 0, 0 })

local function setCull(t, ctx)
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    if ctx:getBool(twoSided) then
        t.rasterization.cullMode = "None"
    end
end

material:technique("DepthOnly", function(t, ctx)
    t.renderTargetFormats = {}
    setCull(t, ctx)
    t.depthTargetFormat = "D32_SFLOAT_S8_UINT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    return true
end)

material:technique("Opaque", function(t, ctx)
    setCull(t, ctx)
    t.depthTargetFormat = "D32_SFLOAT_S8_UINT"
    t.amplificationShader = "Shaders/Passes/MeshPass/MeshCull.as.hlsl"
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.renderTargetFormats = { "B10G11R11_UFLOAT" }
    t.pixelShader = "Shaders/Materials/StandardPBR.ps.hlsl"
    t.depthStencil = Pipeline.DepthStencilState.writeIfEqual()
    return true
end)

return material
