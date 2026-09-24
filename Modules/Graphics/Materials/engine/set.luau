--!strict

local Set = {}
Set.__index = Set

export type Set<T> = typeof(setmetatable(
    {},
    Set
))

function Set.new<T>(items: { T }): Set<T>
	local newSet = {}
	for key, value in items do
		newSet[value] = true
	end
	return setmetatable(newSet, Set)
end

function Set:add(item)
	self[item] = true
end

function Set:remove(item)
	self[item] = nil
end

function Set:contains(item)
	return self[item] == true
end

return Set
