#pragma once
#include <Shaders/Deferred/EvaluateCommon.hlsli>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>

struct MeshEvaluationContext
{
    ReconstructedAttributes m_attributes;
    BufferPointer m_materialParameters;
    BufferPointer m_instanceParameters;
    uint m_shadingRoute;
};


// Resolve the mesh and material together, retaining their parameter pointers for the evaluator.
bool LoadMeshEvaluation(const MeshViewData view, const uint2 visibility, const uint2 pixel, const uint2 resolution,
                        const uint groupIndex, out MeshEvaluationContext result, const bool needUV = true,
                        const bool needTangent = true)
{
    result = (MeshEvaluationContext)0;
    const MeshSceneData scene = LoadMeshScene(view.m_scene);
    if (visibility.x == 0 || (visibility.x >> kObjectTypeShift) != scene.m_visibilityType)
        return false;

    const uint instanceIndex = (visibility.x & kObjectIndexMask) - 1;
    const MeshInstanceTable::Row instance = MeshInstanceTable::Create(scene.m_instances).ReadRow(instanceIndex);
    const MeshGroupTable::Row group = MeshGroupTable::Create(scene.m_groups).ReadRow(groupIndex);

    // Every pixel command belongs to one evaluation binding, but instances of its group may select different LODs.
    const uint firstLod = SurfaceWaveUniform(group.m_lods.Get().m_rowIndex);
    const MeshLodInfoTable::Row lod = MeshLodInfoTable::Create(scene.m_lods).ReadRow(firstLod + DecodeLod(visibility.y));
    const MeshDrawData draw = CreateMeshDrawData(instance, group, lod);
    const float2 pixelCenter = float2(pixel) + 0.5f;
    if (!ReconstructAttributes(view, visibility, pixelCenter, float2(resolution), draw, result.m_attributes, needUV, needTangent))
    {
        return false;
    }

    const uint materialIndex = SurfaceWaveUniform(group.m_materialInstance.Get().m_rowIndex);
    const MaterialInstanceTable::Row material = MaterialInstanceTable::Create(scene.m_materials).ReadRow(materialIndex);
    const uint slot = SurfaceWaveUniform(material.m_shadingSlot.Get());
    result.m_materialParameters.m_deviceAddress = SurfaceWaveUniform(material.m_materialParameters.Get().m_deviceAddress);
    result.m_instanceParameters = draw.m_instance.m_instanceData.Get();
    result.m_shadingRoute = slot < 254 ? slot + 1 : kUnsupportedShadingRoute;
    return true;
}
