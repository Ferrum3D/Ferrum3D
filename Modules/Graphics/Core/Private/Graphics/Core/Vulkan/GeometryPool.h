#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/GeometryPool.h>
#include <Graphics/Core/ResourcePool.h>
#include <festd/bit_vector.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    struct GeometryPool final : public Core::GeometryPool
    {
        FE_RTTI_Class(GeometryPool, "161485C5-B425-4A6A-B65A-0B60C1C60FFE");

        GeometryPool(Core::Device* device, Core::ResourcePool* resourcePool);
        ~GeometryPool() override;

        Core::GeometryHandle Allocate(const Core::GeometryAllocationDesc& desc) override;
        void Free(Core::GeometryHandle handle) override;

        Core::GeometryView GetView(Core::GeometryHandle handle) override;
        Core::MeshletGeometryView GetMeshletView(Core::GeometryHandle handle) override;
        WaitGroup* GetAvailabilityWaitGroup(Core::GeometryHandle handle) override;

    private:
        using GeometryBuffersArray = festd::fixed_vector<Core::Buffer*, Core::Limits::Pipeline::kMaxVertexStreams + 1>;

        struct RegularGeometry final
        {
            Core::GeometryView m_view;
            Core::StreamBufferView m_streamBufferViews[Core::Limits::Pipeline::kMaxVertexStreams];
            GeometryBuffersArray m_buffers;

            void Invalidate()
            {
                m_view = {};
                memset(m_streamBufferViews, 0, sizeof(m_streamBufferViews));
                for (Core::Buffer* buffer : m_buffers)
                    buffer->Release();

                m_buffers.clear();
            }
        };

        struct MeshletGeometry final
        {
            Core::MeshletGeometryView m_view;
            Core::Buffer* m_buffers[4];

            void Invalidate()
            {
                m_view = {};
                for (Core::Buffer*& buffer : m_buffers)
                {
                    if (buffer)
                    {
                        buffer->Release();
                        buffer = nullptr;
                    }
                }
            }
        };

        struct Geometry final
        {
            bool m_isMeshlet = false;
            union
            {
                RegularGeometry m_regular;
                MeshletGeometry m_meshlet;
            };

            ~Geometry() {}

            void Invalidate()
            {
                if (m_isMeshlet)
                    m_meshlet.Invalidate();
                else
                    m_regular.Invalidate();
            }
        };

        Rc<WaitGroup> m_dummyWaitGroup = nullptr;

        Core::ResourcePool* m_resourcePool = nullptr;
        SegmentedVector<Geometry> m_geometries;
        festd::bit_vector m_freeGeometries;
        festd::bit_vector m_allocatedGeometries;
    };
} // namespace FE::Graphics::Vulkan
