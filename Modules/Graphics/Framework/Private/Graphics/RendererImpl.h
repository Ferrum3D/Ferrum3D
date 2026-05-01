#pragma once
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Renderer.h>
#include <festd/vector.h>
#include <memory>

namespace FE::Graphics
{
    struct RendererImpl final : public Renderer
    {
        FE_RTTI("CFD1E397-FC2E-4F9B-99F3-4CF67F695B1E");

        RendererImpl();
        ~RendererImpl() override;

        Scene* CreateScene() override;
        void Render(Scene* scene, Core::Viewport* viewport) override;

        [[nodiscard]] DB::Database* GetDatabase() const
        {
            return m_database.get();
        }

    private:
        void EnsureDatabase();
        void EnsureMainDepthTarget(const Core::ViewportDesc& viewportDesc);
        void SetupFrameGraph(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, Scene& scene, View& view,
                             Core::Viewport& viewport);

        festd::vector<Rc<Scene>> m_scenes;
        festd::unique_ptr<DB::Database> m_database;
        Rc<Core::Device> m_device;
        Rc<Core::Texture> m_mainDepthTarget;
    };
} // namespace FE::Graphics
