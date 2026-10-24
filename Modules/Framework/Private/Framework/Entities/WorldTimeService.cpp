#include <Core/Time/BaseTime.h>
#include <Framework/Entities/WorldTimeService.h>
#include <cmath>

namespace FE::Framework
{
    void WorldTimeService::Init(EntityWorld&)
    {
        m_previousTicks = Platform::GetTicks();
        m_deltaSeconds = 0;
        m_unpausedDeltaSeconds = 0;
        m_elapsedSeconds = 0;
    }


    void WorldTimeService::Update(EntityWorld&)
    {
        const uint64_t ticks = Platform::GetTicks();
        const double elapsed = static_cast<double>(ticks - m_previousTicks) * Platform::GetSecondsPerTick();
        m_previousTicks = ticks;
        Advance(static_cast<float>(elapsed));
    }


    void WorldTimeService::Advance(const float deltaSeconds)
    {
        FE_Assert(std::isfinite(deltaSeconds) && deltaSeconds >= 0);
        m_unpausedDeltaSeconds = deltaSeconds;
        m_deltaSeconds = m_paused ? 0 : m_unpausedDeltaSeconds;
        m_elapsedSeconds += m_deltaSeconds;
    }
} // namespace FE::Framework
