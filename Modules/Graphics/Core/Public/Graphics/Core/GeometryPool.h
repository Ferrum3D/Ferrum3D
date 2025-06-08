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
        Env::Name m_name;
        InputStreamLayout m_inputLayout;
        IndexType m_indexType;
        uint32_t m_indexCount;
        uint32_t m_vertexCount;
    };


    struct GeometryPool : public DeviceObject
    {
        FE_RTTI_Class(GeometryPool, "0DF9A91D-7C49-4E7A-B028-01B66E3C8E0B");

        virtual GeometryHandle Allocate(const GeometryAllocationDesc& desc) = 0;

        virtual void Free(GeometryHandle handle) = 0;

        virtual GeometryView GetView(GeometryHandle handle) = 0;

        virtual WaitGroup* GetAvailabilityWaitGroup(GeometryHandle handle) = 0;
    };
} // namespace FE::Graphics::Core
