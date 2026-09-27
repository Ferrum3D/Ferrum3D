#pragma once
#include <Core/Env/Environment.h>
#include <Core/Math/Matrix4x4.h>
#include <Graphics/Base/Base.h>
#include <Graphics/Base/BaseModuleList.h>
#include <Graphics/Base/DrawTag.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Viewport.h>

namespace FE::Graphics
{
    struct RenderBatchCollector;
    namespace Core
    {
        struct FrameGraph;
        struct RingUploader;
    } // namespace Core

    struct SceneRenderPass final
    {
        Matrix4x4 m_viewProjection;
        Core::Texture* m_colorTarget = nullptr;
        Core::Texture* m_depthTarget = nullptr;
        RectF m_viewport{ kForceInit };
        DrawTag m_drawTag;
        Env::Name m_techniqueRole;
    };

    struct SceneModuleBase : public Memory::RefCountedObjectBase
    {
        FE_RTTI("7729E683-8638-4712-81D3-B1C78B16BFE3");

        ~SceneModuleBase() override = default;

        virtual void Update() {}
        virtual void CollectRenderBatches(RenderBatchCollector&) {}
        virtual void AddRenderPasses(Core::FrameGraph&, Core::RingUploader&, const SceneRenderPass&) {}

    protected:
        explicit SceneModuleBase(Scene* scene)
            : m_scene(scene)
        {
        }

        Scene* m_scene = nullptr;
    };

    using SceneModuleList = BaseModuleList<Scene, SceneModuleBase>;


    struct Scene : public Memory::RefCountedObjectBase
    {
        FE_RTTI("20121F05-8D10-4427-9925-2DBA388379C9");

        ~Scene() override = default;

        [[nodiscard]] Renderer* GetRenderer() const
        {
            return m_renderer;
        }

        [[nodiscard]] SceneModuleList& GetModules()
        {
            return m_moduleList;
        }

        [[nodiscard]] const SceneModuleList& GetModules() const
        {
            return m_moduleList;
        }

        [[nodiscard]] virtual View* CreateView() = 0;
        [[nodiscard]] virtual uint32_t GetViewCount() const = 0;
        [[nodiscard]] virtual View* GetView(uint32_t index) const = 0;

    protected:
        explicit Scene(Renderer* renderer)
            : m_renderer(renderer)
            , m_moduleList(this)
        {
        }

        Renderer* m_renderer = nullptr;
        SceneModuleList m_moduleList;
    };
} // namespace FE::Graphics
