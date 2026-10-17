#pragma once
#include <Shaders/Deferred/EvaluateCommon.hlsli>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>

uint SurfaceShadingRoute(MeshViewData view, uint groupIndex)
{
    const MeshGroupTable::Row group = MeshGroupTable::Create(view.m_scene.Load(0).m_groups).ReadRow(groupIndex);
    const MaterialInstanceTable::Row material =
        MaterialInstanceTable::Create(view.m_scene.Load(0).m_materials).ReadRow(group.m_materialInstance.Get());
    const uint slot = material.m_shadingSlot.Get();
    return slot < 254 ? slot + 1 : kUnsupportedShadingRoute;
}
