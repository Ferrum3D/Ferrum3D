#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Scene/Scene.h>

namespace FE::Graphics
{
    RenderBatchCollector::RenderBatchCollector(std::pmr::memory_resource* allocator, const Matrix4x4& viewProjection)
        : m_viewProjection(viewProjection)
        , m_frustum(viewProjection)
        , m_batches(allocator)
        , m_allocator(allocator)
    {
    }


    void RenderBatchCollector::Collect(Scene& scene)
    {
        scene.GetModules().ForEachActive([this](SceneModuleBase& module) {
            module.CollectRenderBatches(*this);
        });
    }


    bool RenderBatchCollector::IsVisible(const Aabb& bounds) const
    {
        FE_AssertDebug(bounds.IsValid());

        return m_frustum.Overlaps(bounds);
    }


    const Frustum& RenderBatchCollector::GetFrustum() const
    {
        return m_frustum;
    }


    const Matrix4x4& RenderBatchCollector::GetViewProjection() const
    {
        return m_viewProjection;
    }


    RenderBatch& RenderBatchCollector::AddBatch(const Aabb& bounds)
    {
        FE_AssertDebug(IsVisible(bounds));
        RenderBatch& batch = m_batches.emplace_back(m_allocator);
        batch.m_bounds = bounds;
        return batch;
    }
} // namespace FE::Graphics
