#pragma once
#include <Graphics/Core/ComputePipeline.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct ComputePassRegistry final
    {
        [[nodiscard]] uint32_t Register(Core::ComputePipeline* pipeline)
        {
            if (pipeline == nullptr)
                return kInvalidIndex;

            FE_Assert(pipeline->IsReady());
            for (uint32_t index = 0; index < m_pipelines.size(); ++index)
            {
                if (m_pipelines[index].Get() == pipeline)
                    return index;
            }

            FE_Assert(m_pipelines.size() < kInvalidIndex);
            m_pipelines.push_back(Rc<Core::ComputePipeline>(pipeline));
            return m_pipelines.size() - 1;
        }

        [[nodiscard]] festd::span<const Rc<Core::ComputePipeline>> GetPipelines() const
        {
            return m_pipelines;
        }

    private:
        festd::vector<Rc<Core::ComputePipeline>> m_pipelines;
    };
} // namespace FE::Graphics
