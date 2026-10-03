--!strict

local Specializer = require("@engine/specializer")
local TechniqueContext = require("@engine/technique")
local Pipeline = require("@engine/pipeline")

local Material = {}
Material.__index = Material

export type Material = typeof(setmetatable(
    {} :: {
        name: string,
        specializers: { [string]: Specializer.Specializer },
        techniques: { [string]: { [string]: TechniqueContext.TechniqueDesc } },
        parameters: { [string]: { type: string, scope: string, default: { number }? } },
    },
    Material
))

function Material.new(name: string): Material
    local self = setmetatable({
        name = name,
        specializers = {},
        techniques = {},
        parameters = {},
    }, Material)

    return self
end

function Material.specBool(
    self: Material,
    name: string,
    default: boolean
): string
    local specializer = Specializer.new({ 0, 1 }, if default then 1 else 0)
    self.specializers[name] = specializer
    return name
end

function Material.specInt(
    self: Material,
    name: string,
    values: { number },
    default: number
): string
    local specializer = Specializer.new(values, default)
    self.specializers[name] = specializer
    return name
end

function Material.parameter(self: Material, name: string, typeName: string, scope: string, default: { number }?)
    assert(typeName == "Scalar" or typeName == "Vector" or typeName == "Texture2D")
    assert(scope == "Material" or scope == "Instance")
    assert(self.parameters[name] == nil)
    self.parameters[name] = { type = typeName, scope = scope, default = default }
end

local function makeTechniqueKey(specializerNames: { string }, values: { [string]: number }): string
    local orderedValues = {} :: { string }
    for _, name in ipairs(specializerNames) do
        table.insert(orderedValues, `{name}={values[name]}`)
    end

    return table.concat(orderedValues, ";")
end

function Material.technique(
    self: Material,
    role: TechniqueContext.TechniqueRole,
    f: (TechniqueContext.TechniqueDesc, TechniqueContext.TechniqueContext) -> boolean
)
    local specializers = {} :: { Specializer.Specializer }
    local specializerNames = {} :: { string }
    for name, specializer in self.specializers do
        table.insert(specializers, specializer)
        table.insert(specializerNames, name)
    end
    table.sort(specializerNames)
    table.clear(specializers)
    for _, name in specializerNames do
        table.insert(specializers, self.specializers[name])
    end

    local specializerCount = #specializers
    self.techniques[role] = self.techniques[role] or {}

    local specializerValues = {} :: { [string]: number }
    local function traverseVariants(specializerIndex: number)
        if specializerIndex > specializerCount then
            local techniqueContext = TechniqueContext.new(specializerValues)
            local techniqueDesc: TechniqueContext.TechniqueDesc = {
                role = role,
                shaderDefines = {},
                renderTargetFormats = {},
                depthTargetFormat = "Undefined",
                rasterization = Pipeline.RasterizationState.fillNoCull(),
                depthStencil = Pipeline.DepthStencilState.disabled(),
                blend = Pipeline.BlendState.disabled()
            }

            local isValidTechnique = f(techniqueDesc, techniqueContext)
            if isValidTechnique then
                local techniqueKey = makeTechniqueKey(specializerNames, specializerValues)
                self.techniques[role][techniqueKey] = techniqueDesc
            end
            return
        end

        local specializer = specializers[specializerIndex]
        local specializerName = specializerNames[specializerIndex]
        for _, value in ipairs(specializer.values) do
            specializerValues[specializerName] = value
            traverseVariants(specializerIndex + 1)
        end
    end

    traverseVariants(1)
end

return Material
