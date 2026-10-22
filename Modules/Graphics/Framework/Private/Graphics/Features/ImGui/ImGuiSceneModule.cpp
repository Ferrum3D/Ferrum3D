#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Features/ImGui/ImGuiSceneModule.h>
#include <Shaders/Passes/ImGui/ImGui.h>
#include <imgui.h>

namespace FE::Graphics
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(UploadImGuiTexturePipeline, "Shaders/Passes/ImGui/UploadTexture.cs.hlsl");

        // Large atlas updates are split so that a single upload never claims a large part of the ring buffer.
        constexpr uint32_t kMaxUploadChunkBytes = 1024 * 1024;
    } // namespace


    ImGuiSceneModule::ImGuiSceneModule(Scene* scene)
        : SceneModuleBase(scene)
    {
    }


    ImGuiSceneModule::~ImGuiSceneModule()
    {
        FE_Assert(m_textures.empty(), "Call ReleaseTextures before destroying the ImGui scene module");
    }


    void ImGuiSceneModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ImGuiSceneModule::SetFrame(const festd::span<ImTextureData* const> textures)
    {
        m_frameTextures.assign(textures.begin(), textures.end());
        m_drawData.clear();
    }


    void ImGuiSceneModule::SetDrawData(const uint32_t viewportID, const ImDrawData* drawData)
    {
        for (ViewportDrawData& entry : m_drawData)
        {
            if (entry.m_viewportID == viewportID)
            {
                entry.m_drawData = drawData;
                return;
            }
        }

        m_drawData.push_back({ viewportID, drawData });
    }


    const ImDrawData* ImGuiSceneModule::FindDrawData(const uint32_t viewportID) const
    {
        for (const ViewportDrawData& entry : m_drawData)
        {
            if (entry.m_viewportID == viewportID)
                return entry.m_drawData;
        }

        return nullptr;
    }


    void ImGuiSceneModule::ReleaseTextures()
    {
        for (const TextureEntry& entry : m_textures)
        {
            entry.m_data->SetTexID(ImTextureID_Invalid);
            entry.m_data->SetStatus(ImTextureStatus_Destroyed);
            entry.m_data->BackendUserData = nullptr;
        }

        m_textures.clear();
        m_frameTextures.clear();
        m_drawData.clear();
    }


    void ImGuiSceneModule::UpdateRenderData(Core::FrameGraph& graph, Core::RingUploader& uploader)
    {
        FE_PROFILER_ZONE();

        for (ImTextureData* data : m_frameTextures)
        {
            if (data->Status == ImTextureStatus_OK || data->Status == ImTextureStatus_Destroyed)
                continue;

            if (data->Status == ImTextureStatus_WantDestroy)
            {
                // Graph resources and deferred device disposal keep in-flight GPU work valid.
                DestroyTexture(*data);
                continue;
            }

            const bool isCreated = data->Status == ImTextureStatus_WantCreate;
            if (isCreated)
                CreateTexture(graph, *data);

            const TextureEntry* entry = FindTexture(data);
            FE_Assert(entry != nullptr, "ImGui requested an update of a texture that was never created");

            bool isUploaded = true;
            if (isCreated)
            {
                isUploaded = UploadTextureRect(graph, uploader, *entry, RectInt(0, 0, data->Width, data->Height));
            }
            else
            {
                for (const ImTextureRect& update : data->Updates)
                {
                    const RectInt rect(update.x, update.y, update.x + update.w, update.y + update.h);
                    isUploaded &= UploadTextureRect(graph, uploader, *entry, rect);
                }
            }

            // Retry next frame when the upload ring is exhausted; the whole request is replayed.
            if (isUploaded)
                data->SetStatus(ImTextureStatus_OK);
        }
    }


    ImGuiSceneModule::TextureEntry* ImGuiSceneModule::FindTexture(const ImTextureData* data)
    {
        for (TextureEntry& entry : m_textures)
        {
            if (entry.m_data == data)
                return &entry;
        }

        return nullptr;
    }


    void ImGuiSceneModule::CreateTexture(Core::FrameGraph& graph, ImTextureData& data)
    {
        if (FindTexture(&data) != nullptr)
            DestroyTexture(data);

        const Vector2UInt size(static_cast<uint32_t>(data.Width), static_cast<uint32_t>(data.Height));
        Rc texture = Core::Texture::Create(graph.GetDevice(),
                                           Fmt::FormatName("ImGuiTexture{}", data.UniqueID),
                                           Core::Format::kR8G8B8A8_UNORM,
                                           size);

        Core::ResourceCommitParams commitParams;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        graph.GetResourcePool()->CommitTextureMemory(texture.Get(), commitParams);

        data.SetTexID(ToImGuiTextureID(texture.Get()));
        data.BackendUserData = texture.Get();
        m_textures.push_back({ &data, std::move(texture) });
    }


    void ImGuiSceneModule::DestroyTexture(ImTextureData& data)
    {
        const auto found = festd::find_if(m_textures.begin(), m_textures.end(), [&data](const TextureEntry& entry) {
            return entry.m_data == &data;
        });

        if (found != m_textures.end())
            m_textures.erase(found);

        data.SetTexID(ImTextureID_Invalid);
        data.SetStatus(ImTextureStatus_Destroyed);
        data.BackendUserData = nullptr;
    }


    bool ImGuiSceneModule::UploadTextureRect(Core::FrameGraph& graph, Core::RingUploader& uploader, const TextureEntry& entry,
                                             const RectInt rect) const
    {
        ImTextureData& data = *entry.m_data;
        const uint32_t bytesPerPixel = static_cast<uint32_t>(data.BytesPerPixel);
        const uint32_t width = static_cast<uint32_t>(rect.Width());
        const uint32_t rowBytes = width * bytesPerPixel;
        const uint32_t rowsPerChunk = Math::Max(1u, kMaxUploadChunkBytes / rowBytes);

        for (int32_t firstRow = rect.top; firstRow < rect.bottom; firstRow += static_cast<int32_t>(rowsPerChunk))
        {
            const uint32_t rowCount = Math::Min(rowsPerChunk, static_cast<uint32_t>(rect.bottom - firstRow));
            const uint32_t byteSize = AlignUp(rowBytes * rowCount, 4u);

            const Rc staging = Core::Buffer::CreateByteAddress(graph.GetDevice(), "ImGuiTextureUpload", byteSize);
            const bool isUploaded = uploader.UploadGenerated(graph, staging.Get(), byteSize, [&](void* destination) {
                auto* bytes = static_cast<std::byte*>(destination);
                for (uint32_t row = 0; row < rowCount; ++row)
                {
                    const void* source = data.GetPixelsAt(rect.left, firstRow + static_cast<int32_t>(row));
                    memcpy(bytes + row * rowBytes, source, rowBytes);
                }
            });

            if (!isUploaded)
                return false;

            auto* pass = graph.AllocatePassData<ImGuiPass::TextureUploadPassDesc>();
            pass->m_pipeline = UploadImGuiTexturePipeline::GetPipeline();
            pass->m_constants.m_source = graph.GetSRV(staging.Get());
            pass->m_constants.m_target = graph.GetUAV(entry.m_texture.Get());
            pass->m_constants.m_offset = Vector2UInt(static_cast<uint32_t>(rect.left), static_cast<uint32_t>(firstRow));
            pass->m_constants.m_size = Vector2UInt(width, rowCount);
            pass->m_constants.m_bytesPerPixel = bytesPerPixel;
            graph.AddDispatchPass("UploadImGuiTexture", pass, Math::CeilDivide(pass->m_constants.m_size, 8u));
        }

        return true;
    }
} // namespace FE::Graphics
