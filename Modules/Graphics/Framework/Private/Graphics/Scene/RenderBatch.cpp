#include <Graphics/Scene/RenderBatch.h>
#include <Graphics/Scene/Scene.h>

namespace FE::Graphics
{
    RenderBatchCollector::RenderBatchCollector(std::pmr::memory_resource* allocator, const Matrix4x4& viewProjection,
                                               const DrawTag drawTag, const Env::Name techniqueRole)
        : m_viewProjection(viewProjection)
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
            if (homogeneous.w == 0.0f)
            {
                m_frustumBounds = Aabb(Vector3(-Constants::kMaxFloat), Vector3(Constants::kMaxFloat));
                return;
            }
            const Vector3 point(homogeneous.x / homogeneous.w, homogeneous.y / homogeneous.w, homogeneous.z / homogeneous.w);
            m_frustumBounds = Math::Union(m_frustumBounds, point);
        }
        m_frustumBounds.min = m_frustumBounds.min - Vector3(1e-3f);
        m_frustumBounds.max = m_frustumBounds.max + Vector3(1e-3f);
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

        const auto planeTolerance = [](Vector4 plane) {
            return 1e-4f * Math::Sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        };
        const Vector4 planeX(m_viewProjection.m_00, m_viewProjection.m_10, m_viewProjection.m_20, m_viewProjection.m_30);
        const Vector4 planeY(m_viewProjection.m_01, m_viewProjection.m_11, m_viewProjection.m_21, m_viewProjection.m_31);
        const Vector4 planeZ(m_viewProjection.m_02, m_viewProjection.m_12, m_viewProjection.m_22, m_viewProjection.m_32);
        const Vector4 planeW(m_viewProjection.m_03, m_viewProjection.m_13, m_viewProjection.m_23, m_viewProjection.m_33);
        const float leftTolerance = planeTolerance(planeW + planeX);
        const float rightTolerance = planeTolerance(planeW - planeX);
        const float bottomTolerance = planeTolerance(planeW + planeY);
        const float topTolerance = planeTolerance(planeW - planeY);
        const float nearTolerance = planeTolerance(planeZ);
        const float farTolerance = planeTolerance(planeW - planeZ);

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

            outsideLeft &= clip.x < -clip.w - leftTolerance;
            outsideRight &= clip.x > clip.w + rightTolerance;
            outsideBottom &= clip.y < -clip.w - bottomTolerance;
            outsideTop &= clip.y > clip.w + topTolerance;
            outsideNear &= clip.z < -nearTolerance;
            outsideFar &= clip.z > clip.w + farTolerance;
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
