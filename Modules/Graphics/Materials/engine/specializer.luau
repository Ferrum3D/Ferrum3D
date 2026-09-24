--!strict

local Specializer = {}
Specializer.__index = Specializer

export type Specializer = typeof(setmetatable(
    {} :: {
        default: number,
        values: { number }
    },
    Specializer
))

function Specializer.new(values: { number }, default: number): Specializer
    local self = setmetatable({
        default = default,
        values = values,
    }, Specializer)

    return self
end

return Specializer
