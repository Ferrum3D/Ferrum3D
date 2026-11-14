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


// UV-space offset from the pixel to its position in the previous frame, free of projection jitter.
float2 ComputeMeshMotion(const ViewData view, const MeshInstanceTable::Row instance, const ReconstructedAttributes attributes)
{
    const float4x4 previousTransform = Math::AffineToMatrix(instance.m_previousTransform.Get());
    const float4 previousWorld = mul(float4(attributes.m_localPosition, 1), previousTransform);
    const float4 currentClip = mul(float4(attributes.m_worldPosition, 1), view.m_unjitteredViewProjection);
    const float4 previousClip = mul(previousWorld, view.m_previousViewProjection);
    if (currentClip.w <= 0 || previousClip.w <= 0)
        return 0;

    return (previousClip.xy / previousClip.w - currentClip.xy / currentClip.w) * float2(0.5f, -0.5f);
}


// Scaled gradients bias every texture sample of the evaluator by the view's LOD bias.
void ApplyTextureLodBias(const ViewData view, inout ReconstructedAttributes attributes)
{
    const float gradientScale = exp2(view.m_jitterAndLodBias.z);
    attributes.m_uvDdx *= gradientScale;
    attributes.m_uvDdy *= gradientScale;
}


// Resolve the mesh and material together, retaining their parameter pointers for the evaluator.
// Stores the pixel's motion when the pass binds a motion target.
bool LoadMeshEvaluation(const MeshViewData view, const uint2 visibility, const uint2 pixel, const uint2 resolution,
                        const uint groupIndex, out MeshEvaluationContext result, const bool needUV = true,
                        const bool needTangent = true)
{
    result = (MeshEvaluationContext)0;
    const MeshSceneData scene = LoadMeshScene(view.m_scene);
    if (visibility.x == 0 || (visibility.x >> kObjectTypeShift) != scene.m_visibilityType)
        return false;

    const uint instanceIndex = (visibility.x & kObjectIndexMask) - 1;
    const MeshInstanceTable::Row instance = scene.m_instances.ReadRow(instanceIndex);
    const MeshGroupTable::Row group = scene.m_groups.ReadRow(groupIndex);

    // Every pixel command belongs to one evaluation binding, but instances of its group may select different LODs.
    DB::Slice<MeshLodInfoTable> lods = group.m_lods.Get();
    lods.m_rowIndex = SurfaceWaveUniform(lods.m_rowIndex);
    const MeshLodInfoTable::Row lod = scene.m_lods.ReadRow(lods.At(DecodeLod(visibility.y)));
    const MeshDrawData draw = CreateMeshDrawData(instance, group, lod);
    const float2 pixelCenter = float2(pixel) + 0.5f;
    if (!ReconstructAttributes(view, visibility, pixelCenter, float2(resolution), draw, result.m_attributes, needUV, needTangent))
    {
        return false;
    }

    if (GConstants.m_motion.IsValid())
        GConstants.m_motion.Store(pixel, ComputeMeshMotion(view.m_view, instance, result.m_attributes));

    ApplyTextureLodBias(view.m_view, result.m_attributes);

    const uint materialIndex = SurfaceWaveUniform(group.m_materialInstance.Get().m_rowIndex);
    const MaterialInstanceTable::Row material = scene.m_materials.ReadRow(materialIndex);
    const uint slot = SurfaceWaveUniform(material.m_shadingSlot.Get());
    result.m_materialParameters.m_deviceAddress = SurfaceWaveUniform(material.m_materialParameters.Get().m_deviceAddress);
    result.m_instanceParameters = draw.m_instance.m_instanceData.Get();
    result.m_shadingRoute = slot < 254 ? slot + 1 : kUnsupportedShadingRoute;
    return true;
}
