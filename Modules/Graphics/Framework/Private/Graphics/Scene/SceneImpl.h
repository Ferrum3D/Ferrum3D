#pragma once
#include <Graphics/Scene/Scene.h>

namespace FE::Graphics
{
    struct SceneImpl final : public Scene
    {
        FE_RTTI("75336687-960E-4693-AE02-A6DCDA770FAC");

        explicit SceneImpl(Renderer* renderer);
        ~SceneImpl() override;
    };
} // namespace FE::Graphics
