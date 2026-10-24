#pragma once
#include <Framework/Entities/WorldService.h>

namespace FE::Framework
{
    struct WorldTimeService final : WorldService
    {
        FE_RTTI("5275A073-5DCD-400F-A0A4-9753AFD8A171");

        void Init(EntityWorld& world) override;
        void Update(EntityWorld& world) override;

        //! Advance explicitly for deterministic hosts; Update samples the platform clock by default.
        void Advance(float deltaSeconds);

        void SetPaused(bool paused)
        {
            m_paused = paused;
            if (paused)
                m_deltaSeconds = 0;
        }

        [[nodiscard]] bool IsPaused() const
        {
            return m_paused;
        }

        //! Wall-clock frame time for editor tools and other pause-independent work.
        [[nodiscard]] float GetUnpausedDeltaSeconds() const
        {
            return m_unpausedDeltaSeconds;
        }

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
        float m_unpausedDeltaSeconds = 0;
        double m_elapsedSeconds = 0;
        bool m_paused = false;
    };
} // namespace FE::Framework
