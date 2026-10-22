#include <Core/Math/Colors.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Features/ImGui/ImGuiSceneModule.h>
#include <Graphics/Passes/ImGuiPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Shaders/Passes/ImGui/ImGui.h>
#include <imgui.h>

namespace FE::Graphics::ImGuiPass
{
    namespace
    {
        static_assert(sizeof(ImDrawVert) == kVertexByteSize);
        static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t));


        struct ImGuiPipeline final : public Core::GraphicsPipelineVariantSet
        {
            FE_SHADER_SPEC(ColorTargetFormat, Core::Format::kR8G8B8A8_UNORM, Core::Format::kB8G8R8A8_UNORM);
            using Specializer = Core::ShaderSpecializer<ColorTargetFormat>;

            FE_DECLARE_PIPELINE_SET(ImGuiPipeline, Specializer);

        private:
            void SetupRequest(const uint32_t variantIndex, Core::GraphicsPipelineRequest& request) override
            {
                const Specializer specializer(variantIndex);

                Core::TargetColorBlending blending = Core::TargetColorBlending::kDisabled;
                blending.m_blendEnabled = true;
                blending.m_sourceFactor = Core::BlendFactor::kSrcAlpha;
                blending.m_destinationFactor = Core::BlendFactor::kOneMinusSrcAlpha;
                blending.m_sourceAlphaFactor = Core::BlendFactor::kOne;
                blending.m_destinationAlphaFactor = Core::BlendFactor::kOneMinusSrcAlpha;

                request.m_desc.SetTopology(Core::PrimitiveTopology::kTriangleList)
                    .SetVertexShader("Shaders/Passes/ImGui/ImGui.vs.hlsl")
                    .SetPixelShader("Shaders/Passes/ImGui/ImGui.ps.hlsl")
                    .SetRasterization(Core::RasterizationState::kFillNoCull)
                    .SetDepthStencil(Core::DepthStencilState::kDisabled)
                    .SetColorBlend(blending)
                    .SetRTVFormat(specializer.Get<ColorTargetFormat>());
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(ImGuiPipeline);


        struct DrawCommand final
        {
            //! Framebuffer pixels as left, top, right, bottom; clamped to the acquired target when recorded.
            //! Packed because the frame allocator only guarantees scalar alignment.
            PackedVector4F m_clipRect;
            uint32_t m_textureIndex = 0;
            uint32_t m_firstIndex = 0;
            uint32_t m_vertexOffset = 0;
            uint32_t m_indexCount = 0;
        };


        //! Geometry and commands copied while the ImGui draw data is still readable.
        struct CapturedDrawData final
        {
            explicit CapturedDrawData(std::pmr::memory_resource* allocator)
                : m_commands(allocator)
            {
            }

            Rc<Core::Buffer> m_vertices;
            Rc<Core::Buffer> m_indices;
            festd::inline_vector<Rc<Core::Texture>, kMaxPassTextures> m_textures;
            festd::pmr::vector<DrawCommand> m_commands;
            Vector2 m_scale = Vector2::kZero;
            Vector2 m_translation = Vector2::kZero;
        };


        uint32_t FindOrAddTexture(CapturedDrawData& captured, Core::Texture* texture)
        {
            for (uint32_t textureIndex = 0; textureIndex < captured.m_textures.size(); ++textureIndex)
            {
                if (captured.m_textures[textureIndex].Get() == texture)
                    return textureIndex;
            }

            captured.m_textures.push_back(texture);
            return captured.m_textures.size() - 1;
        }


        void CaptureCommands(const ImDrawData& drawData, CapturedDrawData& captured)
        {
            const Vector2 displayPos(drawData.DisplayPos.x, drawData.DisplayPos.y);
            const Vector2 framebufferScale(drawData.FramebufferScale.x, drawData.FramebufferScale.y);

            uint32_t listFirstIndex = 0;
            uint32_t listVertexOffset = 0;
            for (const ImDrawList* drawList : drawData.CmdLists)
            {
                for (const ImDrawCmd& command : drawList->CmdBuffer)
                {
                    // Callbacks would run on a render fiber after the ImGui frame ended, so they are not supported.
                    if (command.UserCallback != nullptr || command.ElemCount == 0)
                        continue;

                    auto* texture = reinterpret_cast<Core::Texture*>(static_cast<uintptr_t>(command.GetTexID()));
                    if (texture == nullptr)
                        continue;

                    const Vector2 clipMin = (Vector2(command.ClipRect.x, command.ClipRect.y) - displayPos) * framebufferScale;
                    const Vector2 clipMax = (Vector2(command.ClipRect.z, command.ClipRect.w) - displayPos) * framebufferScale;

                    DrawCommand& drawCommand = captured.m_commands.emplace_back();
                    drawCommand.m_clipRect = PackedVector4F(clipMin.x, clipMin.y, clipMax.x, clipMax.y);
                    drawCommand.m_textureIndex = FindOrAddTexture(captured, texture);
                    drawCommand.m_firstIndex = listFirstIndex + command.IdxOffset;
                    drawCommand.m_vertexOffset = listVertexOffset + command.VtxOffset;
                    drawCommand.m_indexCount = command.ElemCount;
                }

                listFirstIndex += static_cast<uint32_t>(drawList->IdxBuffer.Size);
                listVertexOffset += static_cast<uint32_t>(drawList->VtxBuffer.Size);
            }
        }


        bool UploadGeometry(Core::FrameGraph& graph, Core::RingUploader& uploader, const ImDrawData& drawData,
                            CapturedDrawData& captured)
        {
            const uint32_t vertexBytes = static_cast<uint32_t>(drawData.TotalVtxCount) * kVertexByteSize;
            const uint32_t indexBytes = AlignUp(static_cast<uint32_t>(drawData.TotalIdxCount * sizeof(ImDrawIdx)), 4u);
            captured.m_vertices = Core::Buffer::CreateByteAddress(graph.GetDevice(), "ImGuiVertices", vertexBytes);
            captured.m_indices = Core::Buffer::CreateByteAddress(graph.GetDevice(), "ImGuiIndices", indexBytes);

            const bool areVerticesUploaded =
                uploader.UploadGenerated(graph, captured.m_vertices.Get(), vertexBytes, [&drawData](void* destination) {
                    auto* bytes = static_cast<std::byte*>(destination);
                    for (const ImDrawList* drawList : drawData.CmdLists)
                    {
                        const size_t byteSize = drawList->VtxBuffer.size_in_bytes();
                        memcpy(bytes, drawList->VtxBuffer.Data, byteSize);
                        bytes += byteSize;
                    }
                });

            if (!areVerticesUploaded)
                return false;

            return uploader.UploadGenerated(graph,
                                            captured.m_indices.Get(),
                                            indexBytes,
                                            [&drawData, indexBytes](void* destination) {
                                                auto* bytes = static_cast<std::byte*>(destination);
                                                std::byte* end = bytes + indexBytes;
                                                for (const ImDrawList* drawList : drawData.CmdLists)
                                                {
                                                    const size_t byteSize = drawList->IdxBuffer.size_in_bytes();
                                                    memcpy(bytes, drawList->IdxBuffer.Data, byteSize);
                                                    bytes += byteSize;
                                                }

                                                // Indices are fetched as 32-bit words, so the padding must be initialized.
                                                memset(bytes, 0, static_cast<size_t>(end - bytes));
                                            });
        }


        RectInt ClampScissor(const PackedVector4F clipRect, const Vector2UInt targetSize)
        {
            const float right = static_cast<float>(targetSize.x);
            const float bottom = static_cast<float>(targetSize.y);
            const int32_t left = static_cast<int32_t>(Math::Clamp(clipRect.x, 0.0f, right));
            const int32_t top = static_cast<int32_t>(Math::Clamp(clipRect.y, 0.0f, bottom));
            return RectInt(left,
                           top,
                           Math::Max(left, static_cast<int32_t>(Math::Clamp(clipRect.z, 0.0f, right))),
                           Math::Max(top, static_cast<int32_t>(Math::Clamp(clipRect.w, 0.0f, bottom))));
        }


        void AddDrawPass(Core::FrameGraph& graph, const CapturedDrawData& captured, const festd::span<const DrawCommand> commands,
                         const ViewOutput& output, const bool clearTarget)
        {
            const Core::Format targetFormat = output.m_target.GetBaseDesc().m_imageFormat;
            ImGuiPipeline::Specializer specializer;
            specializer.Set<ImGuiPipeline::ColorTargetFormat>(targetFormat);

            auto* passDesc = graph.AllocatePassData<DrawPassDesc>();
            passDesc->m_colorTarget = output.m_target;
            passDesc->m_vertices = { captured.m_vertices.Get(),
                                     Core::BarrierSyncFlags::kVertexShading,
                                     Core::BarrierAccessFlags::kShaderRead };
            passDesc->m_indices = { captured.m_indices.Get(),
                                    Core::BarrierSyncFlags::kVertexShading,
                                    Core::BarrierAccessFlags::kShaderRead };

            // Each pass declares at most kMaxPassTextures textures; local slots map to the captured texture list.
            festd::fixed_vector<uint32_t, kMaxPassTextures> textureSlots;
            festd::fixed_vector<TextureSRVDescriptor, kMaxPassTextures> textureDescriptors;
            for (const DrawCommand& command : commands)
            {
                if (festd::find(textureSlots.begin(), textureSlots.end(), command.m_textureIndex) != textureSlots.end())
                    continue;

                const Core::TextureView texture = Core::TextureView::Create(captured.m_textures[command.m_textureIndex].Get());
                passDesc->m_textures[textureSlots.size()] = { texture,
                                                              Core::BarrierSyncFlags::kPixelShading,
                                                              Core::BarrierAccessFlags::kShaderRead,
                                                              Core::BarrierLayout::kShaderRead };
                textureSlots.push_back(command.m_textureIndex);
                textureDescriptors.push_back(graph.GetSRV(texture));
            }

            DrawConstants constants;
            constants.m_scale = captured.m_scale;
            constants.m_translation = captured.m_translation;
            constants.m_vertices = graph.GetSRV(captured.m_vertices.Get());
            constants.m_indices = graph.GetSRV(captured.m_indices.Get());
            constants.m_sampler = graph.GetSampler(Core::SamplerState::kLinearClamp);

            const Vector2UInt targetSize = output.m_target.GetBaseDesc().GetSize2D();
            const Core::GraphicsPipeline* pipeline = ImGuiPipeline::GetPipeline(specializer);
            graph.AddPass("ImGui", passDesc, [=](Core::FrameGraphContext& context) {
                if (clearTarget)
                    context.ClearColorTarget(0, Colors::kBlack);

                context.BeginRenderPass();
                for (const DrawCommand& command : commands)
                {
                    const RectInt scissor = ClampScissor(command.m_clipRect, targetSize);
                    if (scissor.Width() <= 0 || scissor.Height() <= 0)
                        continue;

                    const auto slot = festd::find(textureSlots.begin(), textureSlots.end(), command.m_textureIndex);
                    DrawConstants drawConstants = constants;
                    drawConstants.m_texture = textureDescriptors[static_cast<uint32_t>(slot - textureSlots.begin())];
                    drawConstants.m_firstIndex = command.m_firstIndex;
                    drawConstants.m_vertexOffset = command.m_vertexOffset;

                    context.SetScissor(scissor);
                    context.SetPipeline(pipeline);
                    context.PushConstants(drawConstants);
                    context.Draw(command.m_indexCount, 1, 0, 0);
                }

                context.EndRenderPass();
            });
        }
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ViewModule::Update(Core::FrameGraphBlackboard& blackboard)
    {
        blackboard.Add<ViewModule*>(this);
    }


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        ViewModule* const* viewModule = blackboard.TryGet<ViewModule*>();
        if (viewModule == nullptr)
            return;

        const RendererViewData& view = blackboard.Get<RendererViewData>();
        const auto* sceneModule = view.m_scene->GetModules().TryFind<ImGuiSceneModule>();
        if (sceneModule == nullptr)
            return;

        const ImDrawData* drawData = sceneModule->FindDrawData((*viewModule)->m_viewportID);
        if (drawData == nullptr || drawData->TotalIdxCount == 0)
            return;

        const Vector2 displaySize(drawData->DisplaySize.x, drawData->DisplaySize.y);
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return;

        FE_FG_SCOPE(graph, "ImGuiUpload");

        CapturedDrawData captured{ graph.GetAllocator() };
        if (!UploadGeometry(graph, *view.m_renderQueueUploader, *drawData, captured))
            return;

        // Positions are in ImGui display coordinates; the renderer's viewport transform flips Y.
        const Vector2 displayPos(drawData->DisplayPos.x, drawData->DisplayPos.y);
        captured.m_scale = Vector2(2.0f / displaySize.x, -2.0f / displaySize.y);
        captured.m_translation = Vector2(-1.0f - displayPos.x * captured.m_scale.x, 1.0f - displayPos.y * captured.m_scale.y);
        CaptureCommands(*drawData, captured);

        blackboard.Add<CapturedDrawData>(std::move(captured));
    }


    void AddOutputPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        const CapturedDrawData* captured = blackboard.TryGet<CapturedDrawData>();
        if (captured == nullptr || captured->m_commands.empty())
            return;

        FE_FG_SCOPE(graph, "ImGui");

        ViewOutput& output = blackboard.Get<ViewOutput>();
        bool clearTarget = !output.m_isInitialized;

        // Split the command list wherever a pass would exceed its texture declarations.
        const festd::span<const DrawCommand> commands = captured->m_commands;
        uint32_t firstCommand = 0;
        festd::inline_vector<uint32_t, kMaxPassTextures> passTextures;
        for (uint32_t commandIndex = 0; commandIndex <= commands.size(); ++commandIndex)
        {
            const bool isLast = commandIndex == commands.size();
            if (!isLast)
            {
                const uint32_t textureIndex = commands[commandIndex].m_textureIndex;
                const bool isNewTexture =
                    festd::find(passTextures.begin(), passTextures.end(), textureIndex) == passTextures.end();
                if (!isNewTexture || passTextures.size() < kMaxPassTextures)
                {
                    if (isNewTexture)
                        passTextures.push_back(textureIndex);

                    continue;
                }
            }

            AddDrawPass(graph, *captured, commands.subspan(firstCommand, commandIndex - firstCommand), output, clearTarget);
            clearTarget = false;
            firstCommand = commandIndex;
            passTextures.clear();
            if (!isLast)
                passTextures.push_back(commands[commandIndex].m_textureIndex);
        }

        output.m_isInitialized = true;
    }
} // namespace FE::Graphics::ImGuiPass
