#include <Framework/Entities/EntityWorld.h>
#include <Framework/Entities/WorldTimeService.h>
#include <Framework/Input/Desktop/Backend.h>
#include <Framework/Input/WorldInputService.h>

namespace FE::Input
{
    void WorldInputService::Init(Framework::EntityWorld& world)
    {
        FE_Assert(world.FindService<Framework::WorldTimeService>() != nullptr, "Input requires WorldTimeService before it");
        if (!m_backend)
            m_backend.reset(Memory::DefaultNew<Desktop::Backend>());

        m_frame = 0;
        m_deltaSeconds = 0;
    }


    void WorldInputService::Shutdown(Framework::EntityWorld&)
    {
        m_backend.reset();
    }


    void WorldInputService::Update(Framework::EntityWorld& world)
    {
        m_backend->Update();
        m_deltaSeconds = world.FindService<Framework::WorldTimeService>()->GetDeltaSeconds();
        ++m_frame;
    }


    void WorldInputService::SetBackend(festd::unique_ptr<Core::Backend> backend)
    {
        FE_Assert(backend != nullptr);
        m_backend = std::move(backend);
        ++m_frame;
    }


    festd::span<const ResolvedAction> WorldInputService::Resolve(MappingStack& stack) const
    {
        return stack.Resolve(m_backend->GetKeyboardState(), m_backend->GetMouseState(), m_deltaSeconds, m_frame);
    }
} // namespace FE::Input
