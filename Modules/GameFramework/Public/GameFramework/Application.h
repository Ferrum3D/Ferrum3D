#pragma once
#include <Core/IO/Path.h>
#include <Framework/Application/Application.h>
#include <Framework/Entities/EntityWorldInstance.h>
#include <Graphics/Core/DeviceFactory.h>

namespace FE::Graphics
{
    struct MaterialStreamer;
    struct MeshStreamer;
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
    };


    //! @brief Shared engine startup, world execution and graphics teardown. Own one application at a time.
    struct Application : Framework::Application
    {
        FE_RTTI("aaa69126-4427-4055-b1c4-04013aae10af");

        //! @brief Retain reflection for Core, Framework, Graphics and GameFramework at link time.
        Application();
        //! @brief Destroy after Shutdown has released fiber-dependent services.
        ~Application() override;
        //! @brief Create the window, graphics services and initial world on the main job fiber after Env::Init.
        bool Initialize(const ApplicationSettings& settings);
        //! @brief Run until the window closes, or for a bounded frame count; leaves the job system running for Shutdown.
        int32_t Run(uint32_t maxFrames = 0);
        //! @brief Remove entity membership and release graphics/assets while the job system is still running.
        void Shutdown();
        //! @brief Borrow the initialized world for explicit system registration and entity commands.
        [[nodiscard]] Framework::EntityWorld& GetWorld();

    protected:
        //! @brief Order default transform/extraction phases; override to insert application phases or stages.
        virtual void ScheduleWorldUpdate(Framework::EntityWorld& world);
        //! @brief Tick assets, collect systems and execute the application's world schedule for one frame.
        Rc<WaitGroup> ScheduleUpdate() override;

    private:
        bool LoadWorld(IO::AssetID asset);
        void DoRelease() override;
        Rc<Graphics::Core::Device> m_device;
        Rc<Graphics::Core::Viewport> m_viewport;
        Rc<Graphics::Core::PipelineFactory> m_pipelineFactory;
        festd::unique_ptr<Framework::EntityWorldInstance> m_world;
        Graphics::MaterialStreamer* m_materialStreamer = nullptr;
        Graphics::MeshStreamer* m_meshStreamer = nullptr;
        Graphics::TextureStreamer* m_textureStreamer = nullptr;
    };
} // namespace FE::GameFramework
