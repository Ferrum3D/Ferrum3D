#include <Graphics/Core/Vulkan/GeometryPool.h>

namespace FE::Graphics::Vulkan
{
    GeometryPool::GeometryPool(Core::Device* device, Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_device = device;

        // TODO: async allocation
        m_dummyWaitGroup = WaitGroup::Create(0);
    }


    Core::GeometryHandle GeometryPool::Allocate(const Core::GeometryAllocationDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint32_t freeIndex = m_freeGeometries.find_first();

        auto& geometry = freeIndex == kInvalidIndex ? m_geometries.push_back() : m_geometries[freeIndex];

        if (freeIndex == kInvalidIndex)
        {
            geometry.Invalidate();
            m_freeGeometries.resize(m_geometries.size() + 1, false);
        }

        FE_Assert(desc.m_inputLayout.m_perInstanceStreamsMask == 0, "Not implemented");

        Bit::Traverse(desc.m_inputLayout.m_activeChannelsMask, [&](const uint32_t channelIndex) {
            const uint32_t streamIndex = desc.m_inputLayout.m_channels[channelIndex].m_streamIndex;
            Core::StreamBufferView& streamView = geometry.m_streamBufferViews[streamIndex];
            streamView.m_byteSize += Core::GetFormatSize(desc.m_inputLayout.m_channels[channelIndex].m_format);
        });

        const uint32_t activeStreamMask = desc.m_inputLayout.CalculateActiveStreamMask();
        Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
            Core::StreamBufferView& streamView = geometry.m_streamBufferViews[streamIndex];
            streamView.m_byteOffset = 0;
            streamView.m_byteSize *= desc.m_vertexCount;

            Core::BufferDesc bufferDesc;
            bufferDesc.m_size = streamView.m_byteSize;
            bufferDesc.m_usage = Core::ResourceUsage::kDeviceOnly;
            bufferDesc.m_flags = Core::BindFlags::kVertexBuffer;

            Core::Buffer* buffer = m_resourcePool->CreateBuffer(Fmt::FormatName("VertexBuffer_{}", desc.m_name), bufferDesc);
            geometry.m_buffers.push_back(buffer);

            streamView.m_buffer = buffer;
        });

        if (desc.m_indexCount > 0)
        {
            Core::BufferDesc bufferDesc;
            bufferDesc.m_size = Core::GetIndexByteSize(desc.m_indexType) * desc.m_indexCount;
            bufferDesc.m_usage = Core::ResourceUsage::kDeviceOnly;
            bufferDesc.m_flags = Core::BindFlags::kIndexBuffer;

            Core::Buffer* buffer = m_resourcePool->CreateBuffer(Fmt::FormatName("IndexBuffer_{}", desc.m_name), bufferDesc);
            geometry.m_buffers.push_back(buffer);

            auto& indexView = geometry.m_view.m_indexBufferView;
            indexView.m_buffer = buffer;
            indexView.m_byteOffset = 0;
            indexView.m_indexType = desc.m_indexType;
            indexView.m_byteSize = bufferDesc.m_size;

            geometry.m_view.m_drawArguments.Init(Core::DrawArgumentsIndexed{ 0, 0, desc.m_indexCount });
        }
        else
        {
            geometry.m_view.m_drawArguments.Init(Core::DrawArgumentsLinear{ 0, desc.m_vertexCount });
        }

        if (!Math::IsPowerOfTwo(activeStreamMask + 1))
        {
            uint32_t streamViewIndex = 0;
            Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
                geometry.m_streamBufferViews[streamViewIndex++] = geometry.m_streamBufferViews[streamIndex];
            });
        }

        geometry.m_view.m_streamBufferViews = geometry.m_streamBufferViews;

        return Core::GeometryHandle{ m_geometries.size() - 1 };
    }


    void GeometryPool::Free(const Core::GeometryHandle handle)
    {
        const uint32_t index = handle.m_value;
        m_geometries[index].Invalidate();
        m_freeGeometries.set(index);
    }


    Core::GeometryView GeometryPool::GetView(const Core::GeometryHandle handle)
    {
        return m_geometries[handle.m_value].m_view;
    }


    WaitGroup* GeometryPool::GetAvailabilityWaitGroup(const Core::GeometryHandle handle)
    {
        (void)handle;
        return m_dummyWaitGroup;
    }
} // namespace FE::Graphics::Vulkan
