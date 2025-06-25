#pragma once
#include <Graphics/Core/GeometryView.h>

namespace FE::Graphics::Core
{
    struct GeometryHandle final : public TypedHandle<GeometryHandle, uint32_t>
    {
        static const GeometryHandle kInvalid;
    };

    inline const GeometryHandle GeometryHandle::kInvalid{ kInvalidIndex };


    struct GeometryAllocationDesc final
    {
        Env::Name m_name = Env::Name::kEmpty;
        InputStreamLayout m_inputLayout = InputStreamLayout::kNull;
        IndexType m_indexType = IndexType::kUint16;
        uint32_t m_indexCount = 0;
        uint32_t m_vertexCount = 0;
        uint32_t m_primitiveCount = 0;
        uint32_t m_meshletCount = 0;
    };


    struct GeometryPool : public DeviceObject
    {
        FE_RTTI_Class(GeometryPool, "0DF9A91D-7C49-4E7A-B028-01B66E3C8E0B");

        virtual GeometryHandle Allocate(const GeometryAllocationDesc& desc) = 0;

        virtual void Free(GeometryHandle handle) = 0;

        virtual GeometryView GetView(GeometryHandle handle) = 0;
        virtual MeshletGeometryView GetMeshletView(GeometryHandle handle) = 0;

        virtual WaitGroup* GetAvailabilityWaitGroup(GeometryHandle handle) = 0;
    };
} // namespace FE::Graphics::Core
