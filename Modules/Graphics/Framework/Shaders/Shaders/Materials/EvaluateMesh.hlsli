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
                        out MeshEvaluationContext result, const bool needUV = true, const bool needTangent = true)
{
    result = (MeshEvaluationContext)0;
    if (visibility.x == 0 || (visibility.x >> kObjectTypeShift) != view.m_scene.Load(0).m_visibilityType)
        return false;

    const uint instanceIndex = (visibility.x & kObjectIndexMask) - 1;
    const MeshDrawData draw = ResolveMeshDrawData(view, instanceIndex);
    const float2 pixelCenter = float2(pixel) + 0.5f;
    if (!ReconstructAttributes(view, visibility, pixelCenter, float2(resolution), draw, result.m_attributes, needUV, needTangent))
    {
        return false;
    }

    const MeshSceneData scene = view.m_scene.Load(0);
    const MaterialInstanceTable::Row material =
        MaterialInstanceTable::Create(scene.m_materials).ReadRow(draw.m_group.m_materialInstance.Get());
    const uint slot = material.m_shadingSlot.Get();
    result.m_materialParameters = material.m_materialParameters.Get();
    result.m_instanceParameters = draw.m_instance.m_instanceData.Get();
    result.m_shadingRoute = slot < 254 ? slot + 1 : kUnsupportedShadingRoute;
    return true;
}
