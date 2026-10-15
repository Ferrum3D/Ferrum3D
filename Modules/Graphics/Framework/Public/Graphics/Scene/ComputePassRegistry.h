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
                {
                    ++m_referenceCounts[index];
                    return index;
                }
            }

            for (uint32_t index = 0; index < m_pipelines.size(); ++index)
            {
                if (m_pipelines[index] == nullptr)
                {
                    m_pipelines[index] = pipeline;
                    m_referenceCounts[index] = 1;
                    return index;
                }
            }

            FE_Assert(m_pipelines.size() < kInvalidIndex);
            m_pipelines.push_back(Rc<Core::ComputePipeline>(pipeline));
            m_referenceCounts.push_back(1);
            return m_pipelines.size() - 1;
        }

        void Release(const uint32_t slot)
        {
            if (slot == kInvalidIndex)
                return;

            FE_Assert(slot < m_pipelines.size() && m_referenceCounts[slot] != 0);
            if (--m_referenceCounts[slot] != 0)
                return;

            m_pipelines[slot].Reset();
            while (!m_pipelines.empty() && m_pipelines.back() == nullptr)
            {
                m_pipelines.pop_back();
                m_referenceCounts.pop_back();
            }
        }

        [[nodiscard]] festd::span<const Rc<Core::ComputePipeline>> GetPipelines() const
        {
            return m_pipelines;
        }

    private:
        festd::vector<Rc<Core::ComputePipeline>> m_pipelines;
        festd::vector<uint32_t> m_referenceCounts;
    };
} // namespace FE::Graphics
