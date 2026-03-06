#pragma once
#include <Graphics/Base/Base.h>
#include <Graphics/Base/BaseModuleList.h>

namespace FE::Graphics
{
    struct RendererModuleBase : public Memory::RefCountedObjectBase
    {
        FE_RTTI("273B0F50-B991-4323-A516-6C87BBCD83A1");

        ~RendererModuleBase() override = default;

    protected:
        explicit RendererModuleBase(Renderer* renderer)
            : m_renderer(renderer)
        {
        }

        Renderer* m_renderer = nullptr;
    };

    using RendererModuleList = BaseModuleList<Renderer, RendererModuleBase>;


    struct Renderer : public Memory::RefCountedObjectBase
    {
        FE_RTTI("8666F778-3BBF-44CB-87B6-AA1C7B2089D7");

        ~Renderer() override = default;

        [[nodiscard]] RendererModuleList& GetModules()
        {
            return m_moduleList;
        }

        [[nodiscard]] const RendererModuleList& GetModules() const
        {
            return m_moduleList;
        }

        [[nodiscard]] virtual Scene* CreateScene() = 0;

    protected:
        Renderer()
            : m_moduleList(this)
        {
        }

        RendererModuleList m_moduleList;
    };
} // namespace FE::Graphics
