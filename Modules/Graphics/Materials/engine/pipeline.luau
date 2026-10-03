--!strict

local Set = require("@engine/set")

local Pipeline = {}
Pipeline.__index = Pipeline

---------------------------------------------------------------------------------------------------
-- Attachment Formats

export type RenderTargetFormat = "R8_SINT"
                              | "R8_UINT"
                              | "R8_SNORM"
                              | "R8_UNORM"
                              | "R8G8_SINT"
                              | "R8G8_UINT"
                              | "R8G8_SNORM"
                              | "R8G8_UNORM"
                              | "R8G8B8A8_SINT"
                              | "R8G8B8A8_UINT"
                              | "R8G8B8A8_SNORM"
                              | "R8G8B8A8_UNORM"
                              | "R8G8B8A8_SRGB"
                              | "B8G8R8A8_UNORM"
                              | "B8G8R8A8_SRGB"
                              | "R16_SINT"
                              | "R16_UINT"
                              | "R16_SNORM"
                              | "R16_UNORM"
                              | "R16_SFLOAT"
                              | "R16G16_SINT"
                              | "R16G16_UINT"
                              | "R16G16_SNORM"
                              | "R16G16_UNORM"
                              | "R16G16_SFLOAT"
                              | "R16G16B16A16_SINT"
                              | "R16G16B16A16_UINT"
                              | "R16G16B16A16_SNORM"
                              | "R16G16B16A16_UNORM"
                              | "R16G16B16A16_SFLOAT"
                              | "R32_SINT"
                              | "R32_UINT"
                              | "R32_SFLOAT"
                              | "R32G32_SINT"
                              | "R32G32_UINT"
                              | "R32G32_SFLOAT"
                              | "R32G32B32A32_SINT"
                              | "R32G32B32A32_UINT"
                              | "R32G32B32A32_SFLOAT"
                              | "A2R10G10B10_UINT"
                              | "A2R10G10B10_UNORM"
                              | "B10G11R11_UFLOAT"

export type DepthTargetFormat = "Undefined"
                             | "D16_UNORM"
                             | "D32_SFLOAT"
                             | "D24_UNORM_S8_UINT"
                             | "D32_SFLOAT_S8_UINT"

---------------------------------------------------------------------------------------------------
-- Rasterization State

export type CullingMode = "None"
                        | "Back"
                        | "Front"
                        | "BackAndFront"

export type PolygonMode = "Fill"
                        | "Line"
                        | "Point"

Pipeline.RasterizationState = {}
Pipeline.RasterizationState.__index = Pipeline.RasterizationState

export type RasterizationState = typeof(setmetatable(
    {} :: {
        cullMode: CullingMode,
        polyMode: PolygonMode
    },
    Pipeline.RasterizationState
))

function Pipeline.RasterizationState.fillNoCull(): RasterizationState
    local self = setmetatable({
        cullMode = "None" :: CullingMode,
        polyMode = "Fill" :: PolygonMode
    }, Pipeline.RasterizationState)
    return self
end

function Pipeline.RasterizationState.fillBackCull(): RasterizationState
    local self = setmetatable({
        cullMode = "Back" :: CullingMode,
        polyMode = "Fill" :: PolygonMode
    }, Pipeline.RasterizationState)
    return self
end


---------------------------------------------------------------------------------------------------
-- Depth Stencil State

export type CompareOp = "Never"
                      | "Always"
                      | "Less"
                      | "Equal"
                      | "LessEqual"
                      | "Greater"
                      | "NotEqual"
                      | "GreaterEqual"

Pipeline.DepthStencilState = {}
Pipeline.DepthStencilState.__index = Pipeline.DepthStencilState

export type DepthStencilState = typeof(setmetatable(
    {} :: {
        depthCompareOp: CompareOp,
        depthTestEnabled: boolean,
        depthWriteEnabled: boolean,
        stencilTestEnabled: boolean
    },
    Pipeline.DepthStencilState
))

function Pipeline.DepthStencilState.disabled(): DepthStencilState
    local self = setmetatable({
        depthCompareOp = "Greater" :: CompareOp,
        depthTestEnabled = false,
        depthWriteEnabled = false,
        stencilTestEnabled = false,
    }, Pipeline.DepthStencilState)
    return self
end

function Pipeline.DepthStencilState.writeIfGreater(): DepthStencilState
    local self = setmetatable({
        depthCompareOp = "Greater" :: CompareOp,
        depthTestEnabled = true,
        depthWriteEnabled = true,
        stencilTestEnabled = false,
    }, Pipeline.DepthStencilState)
    return self
end

function Pipeline.DepthStencilState.writeIfEqual(): DepthStencilState
    local self = setmetatable({
        depthCompareOp = "Equal" :: CompareOp,
        depthTestEnabled = true,
        depthWriteEnabled = true,
        stencilTestEnabled = false,
    }, Pipeline.DepthStencilState)
    return self
end


---------------------------------------------------------------------------------------------------
-- Blend State

Pipeline.BlendState = {}
Pipeline.BlendState.__index = Pipeline.BlendState

export type ColorComponent = "R" | "G" | "B" | "A"

export type BlendFactor = "Zero"
                        | "One"
                        | "SrcColor"
                        | "OneMinusSrcColor"
                        | "DstColor"
                        | "OneMinusDstColor"
                        | "SrcAlpha"
                        | "OneMinusSrcAlpha"
                        | "DstAlpha"
                        | "OneMinusDstAlpha"
                        | "ConstantColor"
                        | "OneMinusConstantColor"
                        | "ConstantAlpha"
                        | "OneMinusConstantAlpha"
                        | "SrcAlphaSaturate"
                        | "Src1Color"
                        | "OneMinusSrc1Color"
                        | "Src1Alpha"
                        | "OneMinusSrc1Alpha"

export type BlendOp = "Add"
                    | "Subtract"
                    | "ReverseSubtract"
                    | "Min"
                    | "Max"

export type BlendState = typeof(setmetatable(
    {} :: {
        colorWrite: Set.Set<ColorComponent>,
        sourceFactor: BlendFactor,
        destinationFactor: BlendFactor,
        blendOp: BlendOp,
        sourceAlphaFactor: BlendFactor,
        destinationAlphaFactor: BlendFactor,
        alphaBlendOp: BlendOp,
        blendEnabled: boolean
    },
    Pipeline.BlendState
))

function Pipeline.BlendState.disabled(): BlendState
    local self = setmetatable({
        colorWrite = Set.new<<ColorComponent>>{ "R", "G", "B", "A" },
        sourceFactor = "One" :: BlendFactor,
        destinationFactor = "Zero" :: BlendFactor,
        blendOp = "Add" :: BlendOp,
        sourceAlphaFactor = "One" :: BlendFactor,
        destinationAlphaFactor = "Zero" :: BlendFactor,
        alphaBlendOp = "Add" :: BlendOp,
        blendEnabled = false
    }, Pipeline.BlendState)
    return self
end

return Pipeline
