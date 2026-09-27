#pragma once
#include <Core/Containers/SegmentedVector.h>
#include <Core/Env/Environment.h>
#include <Core/Math/Aabb.h>
#include <Core/Math/Matrix4x4.h>
#include <Graphics/Base/DrawTag.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Shaders/Base/Base.h>

namespace FE::Graphics
{
    struct RenderDraw final
    {
        Matrix4x4 m_viewProjection;
        BufferPointer m_meshInstanceTable;
        BufferPointer m_meshGroupTable;
        BufferPointer m_meshLodInfoTable;
        BufferPointer m_materialInstanceTable;
        uint32_t m_instanceIndex = kInvalidIndex;
        uint32_t m_meshletCount = 0;
        Core::GraphicsPipeline* m_pipeline = nullptr;
    };


    struct RenderBatch final
    {
        explicit RenderBatch(std::pmr::memory_resource* allocator)
            : m_draws(allocator)
        {
        }

        Aabb m_bounds = Aabb::kInvalid;
        SegmentedVector<RenderDraw> m_draws;
    };


    struct Scene;

    struct RenderBatchCollector final
    {
        RenderBatchCollector(std::pmr::memory_resource* allocator, const Matrix4x4& viewProjection, Core::Format colorFormat,
                             DrawTag drawTag, Env::Name techniqueRole);

        void Collect(Scene& scene);
        [[nodiscard]] bool IsVisible(const Aabb& bounds) const;
        [[nodiscard]] const Aabb& GetFrustumBounds() const;
        [[nodiscard]] const Matrix4x4& GetViewProjection() const;
        [[nodiscard]] Core::Format GetColorFormat() const;
        [[nodiscard]] DrawTag GetDrawTag() const;
        [[nodiscard]] Env::Name GetTechniqueRole() const;
        RenderBatch& AddBatch(const Aabb& bounds);

        [[nodiscard]] const SegmentedVector<RenderBatch>& GetBatches() const
        {
            return m_batches;
        }

    private:
        Matrix4x4 m_viewProjection;
        Aabb m_frustumBounds = Aabb::kInvalid;
        Core::Format m_colorFormat = Core::Format::kUndefined;
        DrawTag m_drawTag;
        Env::Name m_techniqueRole;
        SegmentedVector<RenderBatch> m_batches;
        std::pmr::memory_resource* m_allocator = nullptr;
    };
} // namespace FE::Graphics
