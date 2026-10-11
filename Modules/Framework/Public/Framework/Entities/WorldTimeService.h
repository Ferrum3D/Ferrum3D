#pragma once
#include <Framework/Entities/WorldService.h>

namespace FE::Framework
{
    struct WorldTimeService final : WorldService
    {
        FE_RTTI("5275A073-5DCD-400F-A0A4-9753AFD8A171");

        void Init(EntityWorld& world) override;
        void Update(EntityWorld& world) override;
        [[nodiscard]] float GetDeltaSeconds() const
        {
            return m_deltaSeconds;
        }
        [[nodiscard]] double GetElapsedSeconds() const
        {
            return m_elapsedSeconds;
        }

    private:
        uint64_t m_previousTicks = 0;
        float m_deltaSeconds = 0;
        double m_elapsedSeconds = 0;
    };
} // namespace FE::Framework
