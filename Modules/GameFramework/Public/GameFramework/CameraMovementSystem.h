#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <Framework/Input/WorldInputService.h>
#include <GameFramework/CameraMovementComponent.h>
#include <GameFramework/TransformComponents.h>

namespace FE::GameFramework
{
    struct CameraInput final
    {
        Vector2 m_move{ 0, 0 };
        float m_elevate = 0;
        Vector2 m_look{ 0, 0 };
    };


    struct CameraMovementSystem final : public Framework::WorldSystem
    {
        FE_RTTI("9A548BD5-6768-49BC-9FE6-77F418F8C9C5");

        void Init(Framework::EntityWorld& world) override;
        void Shutdown(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;
        static bool ApplyMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                  const CameraInput& input, float deltaSeconds);

    private:
        struct Bindings final
        {
            IO::AssetID m_moveID, m_elevateID, m_lookID;

            Input::ActionHandle<Vector2> m_move;
            Input::ActionHandle<Vector2> m_look;
            Input::ActionHandle<float> m_elevate;
            uint64_t m_frame = 0;
        };

        festd::unordered_dense_map<Uuid, Bindings> m_bindings;
        Input::WorldInputService* m_input = nullptr;
        CameraInput ReadInput(const Framework::Entity& entity, const CameraMovementComponent& movement);
    };
} // namespace FE::GameFramework
