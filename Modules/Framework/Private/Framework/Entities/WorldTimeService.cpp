#include <Core/Time/BaseTime.h>
#include <Framework/Entities/WorldTimeService.h>

namespace FE::Framework
{
    void WorldTimeService::Init(EntityWorld&)
    {
        m_previousTicks = Platform::GetTicks();
        m_deltaSeconds = 0;
        m_elapsedSeconds = 0;
    }


    void WorldTimeService::Update(EntityWorld&)
    {
        const uint64_t ticks = Platform::GetTicks();
        const double elapsed = static_cast<double>(ticks - m_previousTicks) * Platform::GetSecondsPerTick();
        m_previousTicks = ticks;
        m_deltaSeconds = static_cast<float>(elapsed);
        m_elapsedSeconds += elapsed;
    }
} // namespace FE::Framework
