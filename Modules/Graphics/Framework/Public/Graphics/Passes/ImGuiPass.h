#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Scene/View.h>

namespace FE::Graphics::ImGuiPass
{
    inline constexpr uint32_t kMaxPassTextures = 8;


    //! Draw commands issue their own scissors, pipelines and constants; unused texture entries stay null.
    struct DrawPassDesc final
    {
        Core::PassColorTarget m_colorTarget;
        Core::PassBufferAccess m_vertices;
        Core::PassBufferAccess m_indices;
        Core::PassTextureAccess m_textures[kMaxPassTextures];

        FE_RTTI_Reflect("ECA0AC0C-1E76-4F85-87BA-91B8E604C155");
    };


    //! @brief Draw one Dear ImGui viewport published by the scene's ImGuiSceneModule over the view output.
    //!
    //! The view needs no camera or scene targets; it composites over earlier views of the same output or clears it.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("170BA448-4F9D-4793-94B6-EC224B679AC7");

        //! @brief ImGuiViewport::ID of the draw data to render.
        uint32_t m_viewportID = 0;

        explicit ViewModule(View* view);
        ~ViewModule() override;
        void Update(Core::FrameGraphBlackboard& blackboard) override;

    private:
        void DoRelease() override;
    };


    //! @brief Upload the geometry of the view's ImGui viewport while its draw data is readable.
    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);

    //! @brief Draw the uploaded ImGui viewport over the acquired ViewOutput.
    void AddOutputPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::ImGuiPass
