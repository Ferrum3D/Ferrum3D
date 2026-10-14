#pragma once
#include <Shaders/Passes/MeshPass/Geometry.hlsli>
#include <Shaders/Passes/MeshPass/MeshPass.h>

[[vk::push_constant]] Constants GConstants;
static const ViewData GView = GConstants.m_view.Load(0);


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


PixelAttributes LoadAttributes(const MeshDrawData drawData, const uint32_t vertexIndex, const uint32_t instanceIndex)
{
    return TransformAttributes(drawData, vertexIndex, instanceIndex, GView.m_viewProjection);
}


template<typename TMaterial, typename TInstance>
void LoadMaterialParameters(const uint32_t instanceIndex, out TMaterial material, out TInstance instanceData)
{
    MeshInstanceTable instances = MeshInstanceTable::Create(GView.m_instances);
    MeshGroupTable groups = MeshGroupTable::Create(GView.m_groups);
    MaterialInstanceTable materials = MaterialInstanceTable::Create(GView.m_materials);
    const MeshInstanceTable::Row instance = instances.ReadRow(instanceIndex);
    const MeshGroupTable::Row group = groups.ReadRow(instance.m_meshGroup.Get());
    const MaterialInstanceTable::Row materialInstance = materials.ReadRow(group.m_materialInstance.Get());
    material = materialInstance.m_materialParameters.Get().Read<TMaterial>(0);
    instanceData = instance.m_instanceData.Get().Read<TInstance>(0);
}
