#pragma once
#include <GameFramework/TransformationSystem.h>
#include <GameFramework/WorldGraphicsSceneService.h>

namespace FE::GameFramework
{
    namespace Phases
    {
        FE_DECLARE_ENTITY_PHASE(GraphicsExtraction);
    } // namespace Phases


    //! @brief Register camera companions and extract changed camera settings/world transforms after Transformation.
    struct CameraSystem final : Framework::WorldSystem
    {
        FE_RTTI("aaa69126-4427-4055-b1c4-04013aae10a2");

        //! @brief Register authored and transient layouts before materialization.
        void Init(Framework::EntityWorld& world) override;
        //! @brief Record changed extraction and main-thread submission for the application-ordered phase.
        void Update(Framework::EntityUpdateContext& context) override;

    private:
        Framework::ChangeCursor m_changes;
        festd::vector<Framework::EntityID> m_pending;
    };


    //! @brief Register mesh companions and submit changed scene matrices/materials after Transformation.
    struct MeshSystem final : Framework::WorldSystem
    {
        FE_RTTI("aaa69126-4427-4055-b1c4-04013aae10a3");

        //! @brief Register authored and transient layouts before materialization.
        void Init(Framework::EntityWorld& world) override;
        //! @brief Record changed extraction and main-thread submission for the application-ordered phase.
        void Update(Framework::EntityUpdateContext& context) override;

    private:
        Framework::ChangeCursor m_changes;
        festd::vector<Framework::EntityID> m_pending;
    };
} // namespace FE::GameFramework
