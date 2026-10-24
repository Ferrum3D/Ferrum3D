#pragma once
#include <Core/IO/Assets.h>
#include <Core/IO/Path.h>
#include <Framework/Application/Application.h>
#include <Framework/Entities/EntityWorldInstance.h>
#include <Graphics/Core/DeviceFactory.h>

namespace FE::Graphics
{
    struct ImGuiRenderer;
    struct MaterialStreamer;
    struct MeshStreamer;
    struct RenderRequest;
    struct TextureStreamer;

    namespace Core
    {
        struct Device;
        struct PipelineFactory;
        struct Viewport;
    } // namespace Core
} // namespace FE::Graphics

namespace FE::GameFramework
{
    //! @brief Startup content and window settings for the shared engine application.
    struct ApplicationSettings final
    {
        //! @brief Cooked artifact catalog directory.
        IO::Path m_assetDirectory;
        //! @brief Optional initial world; a null ID starts with an empty world.
        IO::AssetID m_world = IO::AssetID::kNull;
        //! @brief Initial client size in pixels.
        Vector2Int m_windowSize{ 1280, 720 };
        //! @brief Graphics backend used to create the device.
        Graphics::Core::GraphicsApi m_graphicsApi = Graphics::Core::GraphicsApi::kVulkan;
        bool m_startPaused = false;
    };


    //! @brief Shared engine application; own one at a time and shut down before stopping the job system.
    struct Application : Framework::Application
    {
        FE_RTTI("2F63D347-123D-4124-A0CB-2AACEE290DC3");

        Application();
        ~Application() override;

        //! @brief Create the window and graphics services, then request the initial world without waiting for assets.
        virtual bool Initialize(const ApplicationSettings& settings);
        void Shutdown();

        //! @brief Run until the window closes, or for a bounded frame count after world readiness; loading frames still render.
        int32_t Run(uint32_t maxFrames = 0);
        [[nodiscard]] Framework::EntityWorld& GetWorld();
        //! Request a replacement world; the current world remains alive while its definition loads.
        bool OpenWorld(IO::AssetID world);
        [[nodiscard]] festd::string_view GetWorldLoadError() const
        {
            return m_worldLoadError;
        }
        [[nodiscard]] bool IsWorldLoading() const
        {
            return m_worldLoadPending;
        }

    protected:
        virtual void PrepareWorldDefinition(Framework::EntityWorldAsset&) {}
        virtual void OnWorldLoaded() {}
        virtual void OnWorldUnloading() {}
        virtual void BeforeWorldUpdate() {}
        void AddImGuiViews(Graphics::RenderRequest& request);
        [[nodiscard]] Graphics::Core::Device* GetDevice() const
        {
            return m_device.Get();
        }
        [[nodiscard]] Graphics::Core::PipelineFactory* GetPipelineFactory() const
        {
            return m_pipelineFactory.Get();
        }
        //! @brief Order default transform/extraction phases; override to insert application phases or stages.
        virtual void ScheduleWorldUpdate(Framework::EntityWorld& world);
        //! @brief Order phases for each fixed step, independently of the per-frame schedule.
        virtual void ScheduleWorldFixedUpdate(Framework::EntityWorld& world);
        //! @brief Collect systems and execute the application's world schedule for one frame.
        Rc<WaitGroup> ScheduleUpdate() override;
        //! @brief Select the views rendered this frame and their targets.
        //!
        //! The default renders the world's camera views into the main viewport, followed by the ImGui viewports.
        virtual void BuildRenderRequest(Graphics::RenderRequest& request);

        [[nodiscard]] Graphics::Core::Viewport* GetMainViewport() const
        {
            return m_viewport.Get();
        }

    private:
        bool UpdateWorldLoading();
        void DoRelease() override;

        Rc<Graphics::Core::Device> m_device;
        Rc<Graphics::Core::Viewport> m_viewport;
        Rc<Graphics::Core::PipelineFactory> m_pipelineFactory;
        festd::unique_ptr<Graphics::ImGuiRenderer> m_imguiRenderer;
        festd::unique_ptr<Framework::EntityWorldInstance> m_world;
        IO::AssetRequest m_worldRequest;
        festd::inline_string m_worldLoadError;
        bool m_worldDefinitionLoaded = false;
        bool m_worldLoadPending = false;
        bool m_startPaused = false;
        uint64_t m_previousUpdateTicks = 0;
        Graphics::MaterialStreamer* m_materialStreamer = nullptr;
        Graphics::MeshStreamer* m_meshStreamer = nullptr;
        Graphics::TextureStreamer* m_textureStreamer = nullptr;
    };
} // namespace FE::GameFramework
