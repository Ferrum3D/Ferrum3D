#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <Shaders/Passes/Visibility/Contracts.h>

// Visibility payloads are private to their producer; generic classification only consumes the binding ID.
uint SurfaceGroup(ByteAddressBufferDescriptor objectData, uint2 visibility)
{
    const MeshViewData view = LoadMeshView(objectData);
    const MeshSceneData scene = view.m_scene.Load(0);
    if ((visibility.x >> kObjectTypeShift) != scene.m_visibilityType)
        return kInvalidIndex;

    const uint instanceIndex = (visibility.x & kObjectIndexMask) - 1;
    const MeshInstanceTable::Row instance = MeshInstanceTable::Create(scene.m_instances).ReadRow(instanceIndex);
    return instance.m_meshGroup.Get().m_rowIndex;
}
