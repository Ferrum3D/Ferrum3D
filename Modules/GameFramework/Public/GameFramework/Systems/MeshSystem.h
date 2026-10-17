#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Phases.h>

namespace FE::GameFramework
{
    //! @brief Register meshes and submit changed scene matrices/materials after Transformation.
    struct MeshSystem final : Framework::WorldSystem
    {
        FE_RTTI("AA875EA6-B101-432B-9D55-BC6A84CDEFE8");

        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;

    private:
        Framework::ChangeCursor m_modelChanges;
        Framework::ChangeCursor m_transformChanges;
    };
} // namespace FE::GameFramework
