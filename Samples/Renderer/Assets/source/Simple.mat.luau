--!strict

local Material = require("@engine/material")
local TechniqueContext = require("@engine/technique")
local Pipeline = require("@engine/pipeline")

local material = Material.new("Simple")
local twoSided = material:specBool("TwoSided", false)
local enabledLayers = material:specInt("EnabledLayers", { 1, 2, 3 }, 1)

local function baseStates(t: TechniqueContext.TechniqueDesc, ctx: TechniqueContext.TechniqueContext)
    t.rasterization = Pipeline.RasterizationState.fillBackCull()
    t.blend = Pipeline.BlendState.disabled()

    if ctx:getBool(twoSided) then
        t.rasterization.cullMode = "None"
    end
end

material:technique("DepthOnly", function(t, ctx)
    baseStates(t, ctx)

    t.depthStencil = Pipeline.DepthStencilState.writeIfGreater()
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"

    return true
end)

material:technique("Opaque", function(t, ctx)
    baseStates(t, ctx)

    t.depthStencil = Pipeline.DepthStencilState.writeIfEqual()
    t.meshShader = "Shaders/Passes/OpaquePass/OpaquePass.ms.hlsl"
    t.pixelShader = "Shaders/Passes/OpaquePass/OpaquePass.ps.hlsl"
    t.shaderDefines["LAYER_COUNT"] = tostring(ctx:getInt(enabledLayers))

    if ctx:getInt(enabledLayers) == 1 then
        t.shaderDefines["SINGLE_LAYER"] = "1"
    end

    if ctx:getBool(twoSided) and ctx:getInt(enabledLayers) == 3 then
        -- impossible permutation
        return false
    end

    return true
end)

return material
