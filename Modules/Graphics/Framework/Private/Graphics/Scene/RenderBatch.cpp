#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Scene/Scene.h>

namespace FE::Graphics
{
    RenderBatchCollector::RenderBatchCollector(std::pmr::memory_resource* allocator, const Matrix4x4& viewProjection,
                                               const Core::Format colorFormat, const DrawTag drawTag,
                                               const Env::Name techniqueRole)
        : m_viewProjection(viewProjection)
        , m_colorFormat(colorFormat)
        , m_drawTag(drawTag)
        , m_techniqueRole(techniqueRole)
        , m_batches(allocator)
        , m_allocator(allocator)
    {
        const Matrix4x4 inverseViewProjection = Math::Invert(viewProjection);
        for (uint32_t corner = 0; corner < 8; ++corner)
        {
            const float x = (corner & 1) != 0 ? 1.0f : -1.0f;
            const float y = (corner & 2) != 0 ? 1.0f : -1.0f;
            const float z = (corner & 4) != 0 ? 1.0f : 0.0f;
            const Vector4 homogeneous = Vector4(x, y, z, 1.0f) * inverseViewProjection;
            const Vector3 point(homogeneous.x / homogeneous.w, homogeneous.y / homogeneous.w, homogeneous.z / homogeneous.w);
            m_frustumBounds.min = Math::Min(m_frustumBounds.min, point);
            m_frustumBounds.max = Math::Max(m_frustumBounds.max, point);
        }
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

        bool outsideLeft = true;
        bool outsideRight = true;
        bool outsideBottom = true;
        bool outsideTop = true;
        bool outsideNear = true;
        bool outsideFar = true;

        for (uint32_t corner = 0; corner < 8; ++corner)
        {
            const float x = (corner & 1) != 0 ? bounds.max.x : bounds.min.x;
            const float y = (corner & 2) != 0 ? bounds.max.y : bounds.min.y;
            const float z = (corner & 4) != 0 ? bounds.max.z : bounds.min.z;
            const Vector4 clip = Vector4(x, y, z, 1.0f) * m_viewProjection;

            outsideLeft &= clip.x < -clip.w;
            outsideRight &= clip.x > clip.w;
            outsideBottom &= clip.y < -clip.w;
            outsideTop &= clip.y > clip.w;
            outsideNear &= clip.z < 0.0f;
            outsideFar &= clip.z > clip.w;
        }

        return !(outsideLeft || outsideRight || outsideBottom || outsideTop || outsideNear || outsideFar);
    }


    const Aabb& RenderBatchCollector::GetFrustumBounds() const
    {
        return m_frustumBounds;
    }


    const Matrix4x4& RenderBatchCollector::GetViewProjection() const
    {
        return m_viewProjection;
    }


    Core::Format RenderBatchCollector::GetColorFormat() const
    {
        return m_colorFormat;
    }


    DrawTag RenderBatchCollector::GetDrawTag() const
    {
        return m_drawTag;
    }


    Env::Name RenderBatchCollector::GetTechniqueRole() const
    {
        return m_techniqueRole;
    }


    RenderBatch& RenderBatchCollector::AddBatch(const Aabb& bounds)
    {
        FE_AssertDebug(IsVisible(bounds));
        RenderBatch& batch = m_batches.emplace_back(m_allocator);
        batch.m_bounds = bounds;
        return batch;
    }
} // namespace FE::Graphics
