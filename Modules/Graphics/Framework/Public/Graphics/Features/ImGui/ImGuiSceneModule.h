#pragma once
#include <Graphics/Core/Texture.h>
#include <Graphics/Scene/Scene.h>
#include <festd/vector.h>

struct ImDrawData;
struct ImTextureData;

namespace FE::Graphics
{
    //! @brief Convert an engine texture to the ImTextureID used by Dear ImGui draw commands of the ImGui renderer.
    //!
    //! The texture must be readable by pixel shaders and outlive the frames that reference it.
    [[nodiscard]] inline uint64_t ToImGuiTextureID(Core::Texture* texture)
    {
        return reinterpret_cast<uintptr_t>(texture);
    }


    //! @brief Dear ImGui content of a scene: textures requested by ImGui and the draw data of each ImGui viewport.
    //!
    //! Publish a frame on the main thread after Renderer::WaitForScene. The renderer reads the published data, including
    //! ImTextureData requests, until it releases scene state, so the next ImGui frame must also start after WaitForScene.
    struct ImGuiSceneModule final : public SceneModuleBase
    {
        FE_RTTI("7F7CB83A-DB68-4BAA-B677-3A54FC30231C");

        explicit ImGuiSceneModule(Scene* scene);
        ~ImGuiSceneModule() override;

        //! @brief Replace the published frame; texture requests are honored even when no viewport renders.
        void SetFrame(festd::span<ImTextureData* const> textures);
        void SetDrawData(uint32_t viewportID, const ImDrawData* drawData);
        [[nodiscard]] const ImDrawData* FindDrawData(uint32_t viewportID) const;

        //! @brief Destroy the GPU textures created for ImGui and mark their requests as destroyed.
        void ReleaseTextures();

        void UpdateRenderData(Core::FrameGraph& graph, Core::RingUploader& uploader) override;

    private:
        struct TextureEntry final
        {
            ImTextureData* m_data = nullptr;
            Rc<Core::Texture> m_texture;
        };


        struct ViewportDrawData final
        {
            uint32_t m_viewportID = 0;
            const ImDrawData* m_drawData = nullptr;
        };

        void DoRelease() override;

        TextureEntry* FindTexture(const ImTextureData* data);
        void CreateTexture(Core::FrameGraph& graph, ImTextureData& data);
        void DestroyTexture(ImTextureData& data);
        [[nodiscard]] bool UploadTextureRect(Core::FrameGraph& graph, Core::RingUploader& uploader, const TextureEntry& entry,
                                             RectInt rect) const;

        festd::vector<ImTextureData*> m_frameTextures;
        festd::inline_vector<ViewportDrawData, 4> m_drawData;
        festd::vector<TextureEntry> m_textures;
    };
} // namespace FE::Graphics
