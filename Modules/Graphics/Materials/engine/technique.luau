--!strict

local Pipeline = require("@engine/pipeline")

local TechniqueContext = {}
TechniqueContext.__index = TechniqueContext

export type TechniqueRole = "DepthOnly" | "Opaque"

export type TechniqueDesc = {
    role: TechniqueRole,

    vertexShader: string?,
    amplificationShader: string?,
    meshShader: string?,
    pixelShader: string?,

    shaderDefines: { [string]: string },
    renderTargetFormats: { Pipeline.RenderTargetFormat },
    depthTargetFormat: Pipeline.DepthTargetFormat,

    rasterization: Pipeline.RasterizationState,
    depthStencil: Pipeline.DepthStencilState,
    blend: Pipeline.BlendState
}

export type TechniqueContext = typeof(setmetatable(
    {} :: {
        specializerValues: { [string]: number }
    },
    TechniqueContext
))

function TechniqueContext.new(specializerValues: { [string]: number }): TechniqueContext
    local self = setmetatable({
        specializerValues = specializerValues,
    }, TechniqueContext)

    return self
end

function TechniqueContext.getBool(self: TechniqueContext, key: string): boolean
    local value = self.specializerValues[key]
    assert(value ~= nil)
    return value ~= 0
end

function TechniqueContext.getInt(self: TechniqueContext, key: string): number
    local value = self.specializerValues[key]
    assert(value ~= nil)
    return value
end

return TechniqueContext
