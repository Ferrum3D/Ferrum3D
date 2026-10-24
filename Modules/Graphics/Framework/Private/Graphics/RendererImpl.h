#pragma once
#include <Graphics/Core/Base.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Viewport.h>
#include <Graphics/Database/Database.h>
#include <Graphics/Materials/ParameterAllocator.h>
#include <Graphics/Renderer.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct RendererImpl final : public Renderer
    {
        FE_RTTI("CFD1E397-FC2E-4F9B-99F3-4CF67F695B1E");

        RendererImpl(Core::Device* device);
        ~RendererImpl() override;

        Scene* CreateScene() override;
        void DestroyScene(Scene* scene) override;
        void Render(RenderRequest request) override;
        void RenderAsync(RenderRequest request) override;
        void WaitForScene() override;
        void WaitForRender() override;

        Core::GraphicsQueue* GetGraphicsQueue() const override;
        Core::AsyncCopyQueue* GetAsyncCopyQueue() const override;
        Core::ResourcePool* GetResourcePool() const override;
        Core::DescriptorManager* GetDescriptorManager() const override;
        MaterialParameterAllocator* GetMaterialParameterAllocator() const override;

        [[nodiscard]] DB::Database* GetDatabase() const
        {
            return m_database.get();
        }

    private:
        struct FrameView final
        {
            View* m_view = nullptr;
            Core::Viewport* m_viewport = nullptr;
            Core::FrameGraphBlackboard* m_blackboard = nullptr;
            Core::Texture* m_texture = nullptr;
        };


        struct FrameViewport final
        {
            Core::Viewport* m_viewport = nullptr;
            bool m_isAcquired = false;
            bool m_isInitialized = false;
        };

        using FrameViewList = festd::inline_vector<FrameView, 8>;
        using FrameViewportList = festd::inline_vector<FrameViewport, 4>;

        void RenderFrame(WaitGroup& sceneReady);
        void PrepareFrameGraph(FrameViewList& views);
        void SetupView(const FrameView& view);
        void AcquireViewports(FrameViewportList& viewports);
        void AddOutputPasses(const FrameViewList& views, FrameViewportList& viewports);

        void DoRelease() override;

        Rc<WaitGroup> m_sceneReady;
        Rc<WaitGroup> m_renderComplete;
        Rc<Core::Device> m_device;
        Rc<Core::ResourcePool> m_resourcePool;
        Rc<Core::GraphicsQueue> m_graphicsQueue;
        Rc<Core::AsyncCopyQueue> m_asyncCopyQueue;
        Rc<Core::DescriptorManager> m_descriptorManager;
        Rc<Core::FrameGraph> m_frameGraph;

        festd::unique_ptr<DB::Database> m_database;
        festd::unique_ptr<MaterialParameterAllocator> m_materialParameters;
        festd::vector<Rc<Scene>> m_scenes;
        //! Retained by the queued frame and released by the caller joining it.
        RenderRequest m_request;
        Core::RingUploader m_renderQueueUploader;
    };
} // namespace FE::Graphics
