#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Input/WorldInputService.h>
#include <GameFramework/CameraMovementComponent.h>
#include <GameFramework/TransformComponents.h>

namespace FE::GameFramework
{
    struct CameraMovementSystem final : Framework::WorldSystem
    {
        FE_RTTI("9A548BD5-6768-49BC-9FE6-77F418F8C9C5");

        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;
        static void ApplyMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                  festd::span<const Input::ResolvedAction> actions, float deltaSeconds);

    private:
        struct TableRuntime final
        {
            IO::AssetID m_id;
            Input::MappingStack m_stack;
            bool m_used = false;
        };

        festd::inline_vector<TableRuntime> m_tables;
        Input::WorldInputService* m_input = nullptr;

        Input::MappingStack& GetStack(const IO::Link<Input::MappingTable>& table);
    };
} // namespace FE::GameFramework
