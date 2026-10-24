#pragma once
#include <Core/Jobs/WaitGroup.h>
#include <Graphics/Base/Base.h>
#include <Graphics/Base/BaseModuleList.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Scene/View.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct MaterialParameterAllocator;


    //! @brief A View and the presentation target receiving its output.
    struct ViewRenderDesc final
    {
        Rc<View> m_view;
        Rc<Core::Viewport> m_viewport;
        Rc<Core::Texture> m_texture;
    };


    //! @brief Views rendered by one frame, in order; later views draw over the output of earlier views.
    //!
    //! Each viewport is acquired and presented once. Pixels that no view writes are cleared to the background color.
    struct RenderRequest final
    {
        festd::inline_vector<Rc<Core::Viewport>, 4> m_viewports;
        festd::inline_vector<ViewRenderDesc, 8> m_views;

        //! @brief Present the viewport this frame, even if no view renders into it.
        void AddViewport(Core::Viewport* viewport);

        //! @brief Render the view into the viewport after the views added before it; disabled views are skipped.
        void AddView(View* view, Core::Viewport* viewport);

        //! @brief Render into a caller-owned texture, e.g. an editor viewport displayed by ImGui.
        void AddView(View* view, Core::Texture* texture);

        //! @brief Render every view of the scene into the viewport; a null scene presents a cleared frame.
        [[nodiscard]] static RenderRequest CreateForScene(Scene* scene, Core::Viewport* viewport);
    };


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

        //! @brief Release renderer ownership of a scene after removing its entity membership.
        virtual void DestroyScene(Scene* scene) = 0;

        //! @brief Render one frame synchronously.
        virtual void Render(RenderRequest request) = 0;

        //! @brief Queue one frame on a job fiber; joins the previous render before reusing renderer state.
        virtual void RenderAsync(RenderRequest request) = 0;

        //! @brief Join scene/asset reads before mutation, lifecycle changes, or asset streamer publication.
        virtual void WaitForScene() = 0;

        //! @brief Join all CPU render work before viewport changes, shutdown, or external queue use.
        virtual void WaitForRender() = 0;

        virtual Core::GraphicsQueue* GetGraphicsQueue() const = 0;
        virtual Core::AsyncCopyQueue* GetAsyncCopyQueue() const = 0;
        virtual Core::ResourcePool* GetResourcePool() const = 0;
        virtual Core::DescriptorManager* GetDescriptorManager() const = 0;
        virtual MaterialParameterAllocator* GetMaterialParameterAllocator() const = 0;

        static void Init(Core::Device* device);
        static void Shutdown();

        static Renderer& Get();

    protected:
        Renderer()
            : m_moduleList(this)
        {
        }

        RendererModuleList m_moduleList;
    };
} // namespace FE::Graphics
