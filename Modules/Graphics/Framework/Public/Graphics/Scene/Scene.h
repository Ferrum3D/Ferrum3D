#pragma once
#include <Graphics/Base/Base.h>
#include <Graphics/Base/BaseModuleList.h>

namespace FE::Graphics
{
    struct SceneModuleBase : public Memory::RefCountedObjectBase
    {
        FE_RTTI("7729E683-8638-4712-81D3-B1C78B16BFE3");

        ~SceneModuleBase() override = default;

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
