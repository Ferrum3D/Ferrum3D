#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/GeometryView.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Core
{
    struct DrawListBuilder final
    {
        explicit DrawListBuilder(std::pmr::memory_resource* linearAllocator, std::pmr::memory_resource* tempAllocator)
            : m_linearAllocator(linearAllocator)
            , m_tempAllocator(tempAllocator)
            , m_geometryViewCache(tempAllocator)
            , m_tempSharedSRGs(m_tempAllocator)
            , m_tempDrawCalls(m_tempAllocator)
        {
        }

        void AddSharedShaderResourceGroup(const ShaderResourceGroup* group)
        {
            m_tempSharedSRGs.push_back(group);
        }

        void AddDrawCall(const DrawCall& drawCall)
        {
            DrawCall& tempDrawCall = m_tempDrawCalls.push_back();
            tempDrawCall.m_instanceOffset = drawCall.m_instanceOffset;
            tempDrawCall.m_instanceCount = drawCall.m_instanceCount;
            tempDrawCall.m_stencilRef = drawCall.m_stencilRef;
            tempDrawCall.m_shaderResourceGroupCount = drawCall.m_shaderResourceGroupCount;
            tempDrawCall.m_unused = 0;

            const auto& pipelineDesc = drawCall.m_pipeline->GetDesc();
            const uint32_t activeStreamCount = pipelineDesc.m_inputLayout.CalculateActiveStreamCount();

            tempDrawCall.m_geometryView = AddGeometryView(drawCall.m_geometryView, activeStreamCount);
            tempDrawCall.m_pipeline = drawCall.m_pipeline;
            tempDrawCall.m_shaderResourceGroups =
                DuplicateArray(drawCall.m_shaderResourceGroups, drawCall.m_shaderResourceGroupCount);
        }

        [[nodiscard]] DrawList Build()
        {
            const uint32_t sharedSRGCount = m_tempSharedSRGs.size();
            auto** sharedSRGs = Memory::AllocateArray<const ShaderResourceGroup*>(m_linearAllocator, sharedSRGCount);
            m_tempSharedSRGs.copy_to(sharedSRGs);

            const uint32_t drawCallCount = m_tempDrawCalls.size();
            auto* drawCalls = Memory::AllocateArray<DrawCall>(m_linearAllocator, drawCallCount);
            m_tempDrawCalls.copy_to(drawCalls);

            DrawList list;
            list.m_sharedShaderResourceGroupCount = sharedSRGCount;
            list.m_sharedShaderResourceGroups = sharedSRGs;
            list.m_drawCallCount = drawCallCount;
            list.m_drawCalls = drawCalls;
            list.m_unused = 0;
            return list;
        }

    private:
        static_assert(std::is_trivially_copyable_v<DrawCall>);

        std::pmr::memory_resource* m_linearAllocator = nullptr;
        std::pmr::memory_resource* m_tempAllocator = nullptr;

        template<class T>
        T* Duplicate(const T* original)
        {
            return Memory::New<T>(m_linearAllocator, *original);
        }

        template<class T>
        T* DuplicateArray(const T* originalArray, const uint32_t count)
        {
            T* array = Memory::AllocateArray<T>(m_linearAllocator, count);
            festd::copy(originalArray, originalArray + count, array);
            return array;
        }

        GeometryView* AddGeometryView(const GeometryView* originalView, const uint32_t streamCount)
        {
            const uint64_t hash = originalView->GetHash(streamCount);
            auto it = m_geometryViewCache.find(hash);
            if (it != m_geometryViewCache.end())
                return it->second;

            GeometryView* view = Duplicate(originalView);
            view->m_streamBufferViews = DuplicateArray(originalView->m_streamBufferViews, streamCount);
            m_geometryViewCache.emplace(hash, view);
            return view;
        }

        festd::pmr::segmented_unordered_dense_map<uint64_t, GeometryView*> m_geometryViewCache;

        SegmentedVector<const ShaderResourceGroup*, 256> m_tempSharedSRGs;
        SegmentedVector<DrawCall> m_tempDrawCalls;
    };
} // namespace FE::Graphics::Core
