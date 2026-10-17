#pragma once
#include <Graphics/Passes/SurfaceWorkBuilder.h>

namespace FE::Graphics
{
    // Object modules provide their own visibility classifier and reconstruction data.
    // Pipeline slots come from the scene registries; binding IDs are local to this batch.
    struct SurfaceEvaluationBatch final
    {
        Rc<Core::Buffer> m_objectData;
        Core::ComputePipeline* m_countPipeline = nullptr;
        Core::ComputePipeline* m_scatterPipeline = nullptr;
        Core::ComputePipeline* m_visibilityDebugPipeline = nullptr;
        Core::ComputePipeline* m_surfaceDebugPipeline = nullptr;
        festd::vector<Visibility::SurfaceBinding> m_bindings;
        uint32_t m_groupRowCount = 0;
        uint32_t m_waveLaneCount = 32;
    };


    using SurfaceEvaluationInputs = festd::inline_vector<SurfaceEvaluationBatch, 1>;
} // namespace FE::Graphics
