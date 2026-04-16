#pragma once
#include <Graphics/Scene/Scene.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct SceneImpl final : public Scene
    {
        FE_RTTI("75336687-960E-4693-AE02-A6DCDA770FAC");

        explicit SceneImpl(Renderer* renderer);
        ~SceneImpl() override;

        View* CreateView() override;
        uint32_t GetViewCount() const override;
        View* GetView(uint32_t index) const override;

    private:
        festd::vector<Rc<View>> m_views;
    };
} // namespace FE::Graphics
