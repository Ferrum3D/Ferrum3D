#include <Graphics/Core/Vulkan/GeometryPool.h>

namespace FE::Graphics::Vulkan
{
    GeometryPool::GeometryPool(Core::Device* device, Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_device = device;
        SetImmediateDestroyPolicy();

        // TODO: async allocation
        m_dummyWaitGroup = WaitGroup::Create(0);
    }


    GeometryPool::~GeometryPool()
    {
        if (Build::IsDevelopment())
        {
            Bit::Traverse(m_allocatedGeometries.view(), [&](const uint32_t index) {
                [[maybe_unused]] auto& geometry = m_geometries[index];
                FE_AssertMsg(false, "Geometry not freed");
            });
        }
    }


    Core::GeometryHandle GeometryPool::Allocate(const Core::GeometryAllocationDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint32_t freeIndex = m_freeGeometries.find_first();

        uint32_t geometryIndex;
        if (freeIndex == kInvalidIndex)
        {
            geometryIndex = m_geometries.size();
            m_geometries.push_back().Invalidate();
            m_freeGeometries.resize(geometryIndex + 1, false);
            m_allocatedGeometries.resize(geometryIndex + 1, true);
        }
        else
        {
            geometryIndex = freeIndex;
            m_freeGeometries.set(freeIndex, false);
            m_allocatedGeometries.set(freeIndex, true);
        }

        const bool isMeshlet = desc.m_meshletCount > 0;

        Core::DrawArguments drawArguments = {};
        Core::IndexBufferView indexBufferView = {};
        Core::StreamBufferView streamBufferViews[Core::Limits::Pipeline::kMaxVertexStreams] = {};
        GeometryBuffersArray geometryBuffers;

        FE_Assert(desc.m_inputLayout.m_perInstanceStreamsMask == 0, "Not implemented");

        Bit::Traverse(desc.m_inputLayout.m_activeChannelsMask, [&](const uint32_t channelIndex) {
            const uint32_t streamIndex = desc.m_inputLayout.m_channels[channelIndex].m_streamIndex;
            Core::StreamBufferView& streamView = streamBufferViews[streamIndex];
            streamView.m_byteSize += Core::GetFormatSize(desc.m_inputLayout.m_channels[channelIndex].m_format);
        });

        const uint32_t activeStreamMask = desc.m_inputLayout.CalculateActiveStreamMask();
        Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
            Core::StreamBufferView& streamView = streamBufferViews[streamIndex];
            streamView.m_byteOffset = 0;
            streamView.m_byteSize *= desc.m_vertexCount;

            Core::BufferDesc bufferDesc;
            bufferDesc.m_size = streamView.m_byteSize;
            bufferDesc.m_usage = Core::ResourceUsage::kDeviceOnly;
            bufferDesc.m_flags = isMeshlet ? Core::BindFlags::kUnorderedAccess : Core::BindFlags::kVertexBuffer;

            Core::Buffer* buffer = m_resourcePool->CreateBuffer(Fmt::FormatName("VertexBuffer_{}", desc.m_name), bufferDesc);
            buffer->AddRef();
            geometryBuffers.push_back(buffer);

            streamView.m_buffer = buffer;
        });

        if (desc.m_indexCount > 0)
        {
            Core::BufferDesc bufferDesc;
            bufferDesc.m_size = Core::GetIndexByteSize(desc.m_indexType) * desc.m_indexCount;
            bufferDesc.m_usage = Core::ResourceUsage::kDeviceOnly;
            bufferDesc.m_flags = isMeshlet ? Core::BindFlags::kUnorderedAccess : Core::BindFlags::kIndexBuffer;

            Core::Buffer* buffer = m_resourcePool->CreateBuffer(Fmt::FormatName("IndexBuffer_{}", desc.m_name), bufferDesc);
            buffer->AddRef();
            geometryBuffers.push_back(buffer);

            indexBufferView.m_buffer = buffer;
            indexBufferView.m_byteOffset = 0;
            indexBufferView.m_indexType = desc.m_indexType;
            indexBufferView.m_byteSize = bufferDesc.m_size;

            drawArguments.Init(Core::DrawArgumentsIndexed{ 0, 0, desc.m_indexCount });
        }
        else
        {
            drawArguments.Init(Core::DrawArgumentsLinear{ 0, desc.m_vertexCount });
        }

        if (!Math::IsPowerOfTwo(activeStreamMask + 1))
        {
            uint32_t streamViewIndex = 0;
            Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
                streamBufferViews[streamViewIndex++] = streamBufferViews[streamIndex];
            });
        }

        auto& geometry = m_geometries[geometryIndex];
        geometry.m_isMeshlet = isMeshlet;

        if (isMeshlet)
        {
            FE_Assert(desc.m_indexCount > 0);
            FE_Assert(desc.m_primitiveCount > 0);
            FE_Assert(geometryBuffers.size() == 2, "Noninterleaved vertex buffers are not supported");

            auto& meshletGeometry = geometry.m_meshlet;
            meshletGeometry.m_buffers[0] = geometryBuffers[0];
            meshletGeometry.m_buffers[1] = geometryBuffers[1];

            meshletGeometry.m_view.m_meshletCount = desc.m_meshletCount;
            meshletGeometry.m_view.m_indexBufferView = indexBufferView;
            meshletGeometry.m_view.m_vertexBufferView = streamBufferViews[0];

            Core::BufferDesc primitivesBufferDesc;
            primitivesBufferDesc.m_size = desc.m_primitiveCount * sizeof(Core::PackedTriangle);
            primitivesBufferDesc.m_usage = Core::ResourceUsage::kDeviceOnly;
            primitivesBufferDesc.m_flags = Core::BindFlags::kUnorderedAccess;

            Core::Buffer* primitivesBuffer =
                m_resourcePool->CreateBuffer(Fmt::FormatName("Primitives_{}", desc.m_name), primitivesBufferDesc);
            primitivesBuffer->AddRef();

            meshletGeometry.m_buffers[2] = primitivesBuffer;
            meshletGeometry.m_view.m_primitiveBufferView.m_buffer = primitivesBuffer;
            meshletGeometry.m_view.m_primitiveBufferView.m_byteOffset = 0;
            meshletGeometry.m_view.m_primitiveBufferView.m_byteSize = primitivesBufferDesc.m_size;

            Core::BufferDesc meshletsBufferDesc;
            meshletsBufferDesc.m_size = desc.m_meshletCount * sizeof(Core::MeshletHeader);
            meshletsBufferDesc.m_usage = Core::ResourceUsage::kDeviceOnly;
            meshletsBufferDesc.m_flags = Core::BindFlags::kUnorderedAccess;

            Core::Buffer* meshletsBuffer =
                m_resourcePool->CreateBuffer(Fmt::FormatName("Meshlets_{}", desc.m_name), meshletsBufferDesc);
            meshletsBuffer->AddRef();

            meshletGeometry.m_buffers[3] = meshletsBuffer;
            meshletGeometry.m_view.m_meshletBufferView.m_buffer = meshletsBuffer;
            meshletGeometry.m_view.m_meshletBufferView.m_byteOffset = 0;
            meshletGeometry.m_view.m_meshletBufferView.m_byteSize = meshletsBufferDesc.m_size;
        }
        else
        {
            FE_Assert(desc.m_primitiveCount == 0);

            auto& regularGeometry = geometry.m_regular;
            regularGeometry.m_buffers = geometryBuffers;
            memcpy(regularGeometry.m_streamBufferViews, streamBufferViews, festd::size_bytes(streamBufferViews));

            regularGeometry.m_view.m_streamBufferViews = regularGeometry.m_streamBufferViews;
            regularGeometry.m_view.m_drawArguments = drawArguments;
            regularGeometry.m_view.m_indexBufferView = indexBufferView;
        }

        return Core::GeometryHandle{ geometryIndex };
    }


    void GeometryPool::Free(const Core::GeometryHandle handle)
    {
        if (!handle.IsValid())
            return;

        const uint32_t index = handle.m_value;
        m_geometries[index].Invalidate();
        m_freeGeometries.set(index);
        m_allocatedGeometries.reset(index);
    }


    Core::GeometryView GeometryPool::GetView(const Core::GeometryHandle handle)
    {
        const auto& geometry = m_geometries[handle.m_value];
        FE_Assert(!geometry.m_isMeshlet);
        return m_geometries[handle.m_value].m_regular.m_view;
    }


    Core::MeshletGeometryView GeometryPool::GetMeshletView(const Core::GeometryHandle handle)
    {
        const auto& geometry = m_geometries[handle.m_value];
        FE_Assert(geometry.m_isMeshlet);
        return m_geometries[handle.m_value].m_meshlet.m_view;
    }


    WaitGroup* GeometryPool::GetAvailabilityWaitGroup(const Core::GeometryHandle handle)
    {
        FE_Unused(handle);
        return m_dummyWaitGroup.Get();
    }
} // namespace FE::Graphics::Vulkan
