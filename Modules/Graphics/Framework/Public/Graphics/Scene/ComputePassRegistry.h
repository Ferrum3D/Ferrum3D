#pragma once
#include <Graphics/Core/ComputePipeline.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct ComputePassRegistry final
    {
        struct Slot final
        {
            Rc<Core::ComputePipeline> m_pipeline;
            uint32_t m_referenceCount = 0;
        };

        [[nodiscard]] uint32_t Register(Core::ComputePipeline* pipeline)
        {
            if (pipeline == nullptr)
                return kInvalidIndex;

            FE_Assert(pipeline->IsReady());
            for (uint32_t index = 0; index < m_slots.size(); ++index)
            {
                if (m_slots[index].m_pipeline.Get() == pipeline)
                {
                    ++m_slots[index].m_referenceCount;
                    return index;
                }
            }

            for (uint32_t index = 0; index < m_slots.size(); ++index)
            {
                if (m_slots[index].m_pipeline == nullptr)
                {
                    ++m_revision;
                    m_slots[index].m_pipeline = pipeline;
                    m_slots[index].m_referenceCount = 1;
                    return index;
                }
            }

            ++m_revision;
            m_slots.push_back({ Rc<Core::ComputePipeline>(pipeline), 1 });
            return m_slots.size() - 1;
        }

        void Release(const uint32_t slot)
        {
            if (slot == kInvalidIndex)
                return;

            FE_Assert(slot < m_slots.size() && m_slots[slot].m_referenceCount != 0);
            if (--m_slots[slot].m_referenceCount != 0)
                return;

            ++m_revision;
            m_slots[slot].m_pipeline.Reset();
            while (!m_slots.empty() && m_slots.back().m_pipeline == nullptr)
                m_slots.pop_back();
        }

        [[nodiscard]] festd::span<const Slot> GetSlots() const
        {
            return m_slots;
        }

        [[nodiscard]] uint32_t GetRevision() const
        {
            return m_revision;
        }

    private:
        uint32_t m_revision = 1;
        festd::vector<Slot> m_slots;
    };
} // namespace FE::Graphics
