#pragma once
#include <Shaders/Passes/MeshPass/Geometry.hlsli>
#include <Shaders/Passes/MeshPass/MeshPass.h>

[[vk::push_constant]] Constants GConstants;
static const MeshViewData GView = GConstants.m_view.Load(0);


//! Each mesh shader group rasterizes one meshlet that survived compute culling.
VisibleMeshlet LoadVisibleMeshlet(const uint32_t groupIndex)
{
    const MeshCommandRange command = GView.m_commands.Load(GConstants.m_commandIndex);
    return GView.m_visibleMeshlets.Load(command.m_firstMeshlet + groupIndex);
}


MeshDrawData LoadMeshDrawData(const uint32_t instanceIndex, const uint32_t lodIndex)
{
    return ResolveMeshDrawData(GView, instanceIndex, lodIndex);
}
