#pragma once
#include <Shaders/Passes/MeshPass/Geometry.hlsli>
#include <Shaders/Passes/MeshPass/MeshPass.h>

[[vk::push_constant]] Constants GConstants;
static const MeshViewData GView = GConstants.m_view.Load(0);


struct MeshPayload
{
    uint m_instanceId;
    uint m_lodId;
    uint m_meshletIds[kMeshletsPerWorkChunk];
};


MeshDrawData LoadMeshDrawData(const uint32_t instanceIndex, const uint32_t lodIndex = kInvalidIndex)
{
    return ResolveMeshDrawData(GView, instanceIndex, lodIndex);
}
