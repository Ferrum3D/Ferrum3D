#include <Core/Time/BaseTime.h>
#include <Graphics/Core/GraphicsQueue.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Passes/ForwardTransparencyPass.h>
#include <Graphics/Passes/Fsr3UpscalerPass.h>
#include <Graphics/Passes/RendererPassCommon.h>
#include <Graphics/Passes/SurfaceEvaluationPass.h>
#include <Graphics/Passes/VisibilityPass.h>
#include <Graphics/Renderer.h>
#include <Shaders/Passes/Fsr3/Fsr3.h>
#include <cmath>

namespace FE::Graphics::Fsr3
{
    //! Resources and camera state carried from one frame of a view to the next.
    struct History final
    {
        Rc<Core::Texture> m_color[2];
        Rc<Core::Texture> m_luma[2];
        Rc<Core::Texture> m_lumaHistory[2];
        Rc<Core::Texture> m_accumulation[2];
        Rc<Core::Texture> m_frameInfo;
        Vector2UInt m_renderSize = Vector2UInt::kZero;
        Vector2UInt m_outputSize = Vector2UInt::kZero;
        Matrix4x4 m_previousViewProjection = Matrix4x4::kIdentity;
        Vector2 m_previousJitter = Vector2::kZero;
        uint64_t m_lastFence = 0;
        uint64_t m_previousTicks = 0;
        uint32_t m_frameIndex = 0;
        bool m_isValid = false;
    };


    namespace
    {
        FE_COMPUTE_PIPELINE_SET(PrepareInputsPipeline, "Shaders/Passes/Fsr3/PrepareInputs.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(LumaPyramidPipeline, "Shaders/Passes/Fsr3/LumaPyramid.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ShadingChangePyramidPipeline, "Shaders/Passes/Fsr3/ShadingChangePyramid.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ShadingChangePipeline, "Shaders/Passes/Fsr3/ShadingChange.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(PrepareReactivityPipeline, "Shaders/Passes/Fsr3/PrepareReactivity.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(LumaInstabilityPipeline, "Shaders/Passes/Fsr3/LumaInstability.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(AccumulatePipeline, "Shaders/Passes/Fsr3/Accumulate.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ClearPipeline, "Shaders/Passes/Fsr3/Clear.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ClearUintPipeline, "Shaders/Passes/Fsr3/ClearUint.cs.hlsl");

        constexpr uint32_t kMinSize = 8;
        constexpr float kMinRenderScale = 1.0f / 3.0f;
        constexpr float kDefaultRenderScale = 2.0f / 3.0f;
        constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
        constexpr uint32_t kBaseJitterPhaseCount = 8;
        constexpr uint32_t kSpdTileSize = 64;
        constexpr uint32_t kSpdMipCount = 6;
        constexpr uint32_t kMaxSpdMipCount = 12;


        //! Frame-local upscaler inputs published by ViewModule::PrepareView.
        struct PassData final
        {
            History* m_history = nullptr;
            Parameters m_parameters{};
            bool m_hasHistory = false;
        };


        //! Inputs, transient targets and history slots shared by the stages of one frame.
        struct FrameResources final
        {
            ConstantsPtr<Parameters> m_parameters;
            SamplerDescriptor m_linearClamp;
            Vector2UInt m_renderSize = Vector2UInt::kZero;
            Vector2UInt m_halfSize = Vector2UInt::kZero;
            Vector2UInt m_outputSize = Vector2UInt::kZero;

            Core::Texture* m_color = nullptr;
            Core::Texture* m_depth = nullptr;
            Core::Texture* m_motion = nullptr;
            Core::Texture* m_reactiveMask = nullptr;
            Core::Texture* m_compositionMask = nullptr;

            const History* m_history = nullptr;
            uint32_t m_current = 0;
            uint32_t m_previous = 1;

            Rc<Core::Texture> m_reconstructedDepth;
            Rc<Core::Texture> m_dilatedDepth;
            Rc<Core::Texture> m_dilatedMotion;
            Rc<Core::Texture> m_farthestDepth;
            Rc<Core::Texture> m_farthestDepthMip;
            Rc<Core::Texture> m_shadingChange;
            Rc<Core::Texture> m_reactiveMasks;
            Rc<Core::Texture> m_newLocks;
            Rc<Core::Texture> m_lumaInstability;
            Rc<Core::Texture> m_spdAtomic;
            Rc<Core::Texture> m_spdPyramid;
            Rc<Core::Texture> m_defaultMask;
            Rc<Core::Texture> m_output;
        };


        float Halton(uint32_t index, const uint32_t base)
        {
            float value = 0.0f;
            float fraction = 1.0f;
            while (index > 0)
            {
                fraction /= static_cast<float>(base);
                value += fraction * static_cast<float>(index % base);
                index /= base;
            }

            return value;
        }


        bool CanUpscale(const View& view, const Vector2UInt outputSize)
        {
            const ViewModuleList& modules = view.GetModules();
            if (!modules.IsActive<SurfaceEvaluation::ViewModule>() || !modules.IsActive<Visibility::ViewModule>())
                return false;

            // Debug visualizations replace the lit color that the upscaler accumulates.
            if (view.IsOrthographic() || view.GetDebugMode().IsValid())
                return false;

            return outputSize.x >= kMinSize && outputSize.y >= kMinSize;
        }


        Vector2UInt CalculateRenderSize(const float renderScale, const Vector2UInt outputSize)
        {
            const float scale =
                std::isfinite(renderScale) ? Math::Clamp(renderScale, kMinRenderScale, 1.0f) : kDefaultRenderScale;
            const uint32_t width = static_cast<uint32_t>(static_cast<float>(outputSize.x) * scale);
            const uint32_t height = static_cast<uint32_t>(static_cast<float>(outputSize.y) * scale);
            return Vector2UInt(Math::Max(kMinSize, width), Math::Max(kMinSize, height));
        }


        uint32_t CalculateJitterPhaseCount(const Vector2UInt renderSize, const Vector2UInt outputSize)
        {
            const float ratio = static_cast<float>(outputSize.x) / static_cast<float>(renderSize.x);
            return Math::Max(1u, static_cast<uint32_t>(static_cast<float>(kBaseJitterPhaseCount) * ratio * ratio));
        }


        //! Sub-pixel offset in render pixels, in the range [-0.5, 0.5].
        Vector2 CalculateJitter(const uint32_t frameIndex, const uint32_t phaseCount)
        {
            const uint32_t phase = frameIndex % phaseCount + 1;
            return Vector2(Halton(phase, 2) - 0.5f, Halton(phase, 3) - 0.5f);
        }


        void ApplyJitter(ViewData& camera, const Vector2 jitter, const Vector2UInt renderSize, const Vector2UInt outputSize)
        {
            Matrix4x4 jitterTransform = Matrix4x4::kIdentity;
            jitterTransform.m_30 = 2.0f * jitter.x / static_cast<float>(renderSize.x);
            jitterTransform.m_31 = -2.0f * jitter.y / static_cast<float>(renderSize.y);

            camera.m_viewProjection = camera.m_unjitteredViewProjection * jitterTransform;
            camera.m_inverseViewProjection = Math::Invert(camera.m_viewProjection);

            // FSR recommends one mip of sharpening beyond the resolution ratio.
            const float lodBias = std::log2(static_cast<float>(renderSize.x) / static_cast<float>(outputSize.x)) - 1.0f;
            camera.m_jitterAndLodBias = PackedVector4F(jitter.x, jitter.y, lodBias, 0.0f);
        }


        Parameters BuildParameters(const View& view, const History& history, const Vector2 jitter, const Vector2 previousJitter,
                                   const float deltaTime)
        {
            const Vector2UInt renderSize = history.m_renderSize;
            const Vector2UInt outputSize = history.m_outputSize;
            const Vector2UInt spdGroups = Math::CeilDivide(renderSize, kSpdTileSize);
            const Matrix4x4& projection = view.GetProjectionMatrix();

            Parameters parameters{};
            parameters.m_renderSize = Vector2Int(renderSize);
            parameters.m_previousFrameRenderSize = parameters.m_renderSize;
            parameters.m_maxRenderSize = parameters.m_renderSize;
            parameters.m_upscaleSize = Vector2Int(outputSize);
            parameters.m_previousFrameUpscaleSize = parameters.m_upscaleSize;
            parameters.m_maxUpscaleSize = parameters.m_upscaleSize;
            parameters.m_downscaleFactor = Vector2(renderSize) / Vector2(outputSize);

            parameters.m_jitter = jitter;
            parameters.m_previousFrameJitter = previousJitter;
            parameters.m_jitterSequenceLength = static_cast<float>(CalculateJitterPhaseCount(renderSize, outputSize));
            parameters.m_motionVectorScale = Vector2(1, 1);
            parameters.m_frameIndex = static_cast<float>(history.m_frameIndex);
            parameters.m_deltaTime = deltaTime;

            // Reversed-Z perspective projection: depth = m_22 + m_32 / viewZ.
            parameters.m_deviceToViewDepth =
                PackedVector4F(projection.m_22, projection.m_32, 1.0f / projection.m_00, 1.0f / projection.m_11);
            parameters.m_tanHalfFOV = 1.0f / projection.m_11;
            parameters.m_viewSpaceToMetersFactor = 1.0f;

            // The fixed exposure never changes between frames.
            parameters.m_deltaPreExposure = 1.0f;
            parameters.m_velocityFactor = 1.0f;
            parameters.m_reactivenessScale = 1.0f;
            parameters.m_shadingChangeScale = 1.0f;
            parameters.m_accumulationAddedPerFrame = 1.0f / 3.0f;
            parameters.m_minDisocclusionAccumulation = -1.0f / 3.0f;

            parameters.m_mipCount = Math::Clamp(Math::FloorLog2(Math::Max(renderSize.x, renderSize.y)), 1u, kMaxSpdMipCount);
            parameters.m_numWorkGroups = spdGroups.x * spdGroups.y;
            return parameters;
        }


        Rc<Core::Texture> CreateHistoryTexture(Core::FrameGraph& graph, const Env::Name name, const Core::Format format,
                                               const Vector2UInt size)
        {
            Rc texture = Core::Texture::Create(graph.GetDevice(), name, format, size);

            Core::ResourceCommitParams commit;
            commit.m_memory = Core::ResourceMemory::kDeviceLocal;
            commit.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
            graph.GetResourcePool()->CommitTextureMemory(texture.Get(), commit);
            return texture;
        }


        void CreateHistoryTextures(Core::FrameGraph& graph, History& history)
        {
            const Vector2UInt renderSize = history.m_renderSize;
            const Vector2UInt outputSize = history.m_outputSize;
            for (uint32_t i = 0; i < 2; ++i)
            {
                history.m_color[i] = CreateHistoryTexture(graph,
                                                          Fmt::FormatName("Fsr3ColorHistory{}", i),
                                                          Core::Format::kR16G16B16A16_SFLOAT,
                                                          outputSize);
                history.m_luma[i] =
                    CreateHistoryTexture(graph, Fmt::FormatName("Fsr3Luma{}", i), Core::Format::kR16_SFLOAT, renderSize);
                history.m_lumaHistory[i] = CreateHistoryTexture(graph,
                                                                Fmt::FormatName("Fsr3LumaHistory{}", i),
                                                                Core::Format::kR16G16B16A16_SFLOAT,
                                                                renderSize);
                history.m_accumulation[i] =
                    CreateHistoryTexture(graph, Fmt::FormatName("Fsr3Accumulation{}", i), Core::Format::kR8_UNORM, renderSize);
            }

            history.m_frameInfo = CreateHistoryTexture(graph, "Fsr3FrameInfo", Core::Format::kR32G32B32A32_SFLOAT, { 1, 1 });
        }


        void Clear(Core::FrameGraph& graph, Core::Texture* target, const PackedVector4F value = {})
        {
            ClearConstants constants{};
            constants.m_target = graph.GetUAV(target);
            constants.m_size = target->GetDesc().GetSize2D();
            constants.m_value = value;
            graph.AddDispatch("ClearFsr3", ClearPipeline::GetPipeline(), constants, Math::CeilDivide(constants.m_size, 8u));
        }


        void ClearUint(Core::FrameGraph& graph, Core::Texture* target)
        {
            ClearUintConstants constants{};
            constants.m_target = graph.GetUAV(target);
            constants.m_size = target->GetDesc().GetSize2D();
            graph.AddDispatch("ClearFsr3Uint",
                              ClearUintPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(constants.m_size, 8u));
        }


        void ClearHistory(Core::FrameGraph& graph, const History& history)
        {
            for (uint32_t i = 0; i < 2; ++i)
            {
                Clear(graph, history.m_color[i].Get());
                Clear(graph, history.m_luma[i].Get());
                Clear(graph, history.m_lumaHistory[i].Get());
                Clear(graph, history.m_accumulation[i].Get());
            }

            Clear(graph, history.m_frameInfo.Get(), PackedVector4F(1, 1e8f, 0, 0));
        }


        Core::TextureView Mip(Core::Texture* texture, const uint32_t mip)
        {
            return Core::TextureView::Create(texture, Core::TextureSubresource::Create(texture->GetDesc(), mip, 0));
        }


        FrameResources CreateFrameResources(Core::FrameGraph& graph, const PassData& data)
        {
            const History& history = *data.m_history;
            const Vector2UInt renderSize = history.m_renderSize;
            const Vector2UInt outputSize = history.m_outputSize;
            const Vector2UInt halfSize(Math::Max(1u, renderSize.x / 2), Math::Max(1u, renderSize.y / 2));

            // The SPD passes address their first six mips unconditionally.
            const Vector2UInt pyramidSize(Math::Max(1u << (kSpdMipCount - 1), halfSize.x),
                                          Math::Max(1u << (kSpdMipCount - 1), halfSize.y));
            const uint32_t pyramidMipCount = Math::FloorLog2(Math::Max(pyramidSize.x, pyramidSize.y)) + 1;

            FrameResources frame;
            frame.m_parameters = graph.CreateConstants(data.m_parameters);
            frame.m_linearClamp = graph.GetSampler(Core::SamplerState::kLinearClamp);
            frame.m_renderSize = renderSize;
            frame.m_halfSize = halfSize;
            frame.m_outputSize = outputSize;
            frame.m_history = &history;
            frame.m_current = history.m_frameIndex & 1u;
            frame.m_previous = frame.m_current ^ 1u;

            Core::Device* device = graph.GetDevice();
            frame.m_reconstructedDepth =
                Core::Texture::Create(device, "Fsr3ReconstructedDepth", Core::Format::kR32_UINT, renderSize);
            frame.m_dilatedDepth = Core::Texture::Create(device, "Fsr3DilatedDepth", Core::Format::kR32_SFLOAT, renderSize);
            frame.m_dilatedMotion = Core::Texture::Create(device, "Fsr3DilatedMotion", Core::Format::kR16G16_SFLOAT, renderSize);
            frame.m_farthestDepth = Core::Texture::Create(device, "Fsr3FarthestDepth", Core::Format::kR16_SFLOAT, renderSize);
            frame.m_farthestDepthMip = Core::Texture::Create(device, "Fsr3FarthestDepthMip", Core::Format::kR16_SFLOAT, halfSize);
            frame.m_shadingChange = Core::Texture::Create(device, "Fsr3ShadingChange", Core::Format::kR8_UNORM, halfSize);
            frame.m_reactiveMasks = Core::Texture::Create(device, "Fsr3ReactiveMasks", Core::Format::kR8G8B8A8_UNORM, renderSize);
            frame.m_newLocks = Core::Texture::Create(device, "Fsr3NewLocks", Core::Format::kR8_UNORM, outputSize);
            frame.m_lumaInstability = Core::Texture::Create(device, "Fsr3LumaInstability", Core::Format::kR16_SFLOAT, renderSize);
            frame.m_spdAtomic = Core::Texture::Create(device, "Fsr3SpdAtomic", Core::Format::kR32_UINT, { 1, 1 });
            frame.m_spdPyramid =
                Core::Texture::Create(device, "Fsr3Pyramid", Core::Format::kR16G16_SFLOAT, pyramidSize, pyramidMipCount);
            frame.m_defaultMask = Core::Texture::Create(device, "Fsr3DefaultMask", Core::Format::kR8_UNORM, { 1, 1 });
            frame.m_output = Core::Texture::Create(device, "Fsr3Output", Core::Format::kR16G16B16A16_SFLOAT, outputSize);
            return frame;
        }


        //! Missing masks fall back to a cleared 1x1 texture.
        void SelectMasks(Core::FrameGraphBlackboard& blackboard, FrameResources& frame)
        {
            frame.m_reactiveMask = frame.m_defaultMask.Get();
            frame.m_compositionMask = frame.m_defaultMask.Get();

            if (const auto* transparency = blackboard.TryGet<ForwardTransparency::Output>())
                frame.m_reactiveMask = transparency->m_reactive.Get();

            const Masks* masks = blackboard.TryGet<Masks>();
            if (masks != nullptr && masks->m_transparencyAndComposition)
                frame.m_compositionMask = masks->m_transparencyAndComposition.Get();
        }


        void ClearFrameResources(Core::FrameGraph& graph, const FrameResources& frame)
        {
            ClearUint(graph, frame.m_reconstructedDepth.Get());
            ClearUint(graph, frame.m_spdAtomic.Get());
            Clear(graph, frame.m_defaultMask.Get());
            Clear(graph, frame.m_newLocks.Get());
        }


        void AddPrepareInputs(Core::FrameGraph& graph, const FrameResources& frame)
        {
            PrepareInputsConstants constants{};
            constants.m_parameters = frame.m_parameters;
            constants.m_linearClamp = frame.m_linearClamp;
            constants.m_rInputDepth = graph.GetSRV(frame.m_depth);
            constants.m_rInputColorJittered = graph.GetSRV(frame.m_color);
            constants.m_rInputMotionVectors = graph.GetSRV(frame.m_motion);
            constants.m_rwFarthestDepth = graph.GetUAV(frame.m_farthestDepth.Get());
            constants.m_rwCurrentLuma = graph.GetUAV(frame.m_history->m_luma[frame.m_current].Get());
            constants.m_rwReconstructedPreviousNearestDepth = graph.GetUAV(frame.m_reconstructedDepth.Get());
            constants.m_rwDilatedDepth = graph.GetUAV(frame.m_dilatedDepth.Get());
            constants.m_rwDilatedMotionVectors = graph.GetUAV(frame.m_dilatedMotion.Get());
            graph.AddDispatch("PrepareInputs",
                              PrepareInputsPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(frame.m_renderSize, 8u));
        }


        void AddLumaPyramid(Core::FrameGraph& graph, const FrameResources& frame)
        {
            Core::Texture* pyramid = frame.m_spdPyramid.Get();

            LumaPyramidConstants constants{};
            constants.m_parameters = frame.m_parameters;
            constants.m_linearClamp = frame.m_linearClamp;
            constants.m_rFarthestDepth = graph.GetSRV(frame.m_farthestDepth.Get());
            constants.m_rwFarthestDepthMip1 = graph.GetUAV(frame.m_farthestDepthMip.Get());
            constants.m_rCurrentLuma = graph.GetSRV(frame.m_history->m_luma[frame.m_current].Get());
            constants.m_rwFrameInfo = graph.GetUAV(frame.m_history->m_frameInfo.Get());
            constants.m_rwSpdMip0 = graph.GetUAV(Mip(pyramid, 0));
            constants.m_rwSpdMip1 = graph.GetUAV(Mip(pyramid, 1));
            constants.m_rwSpdMip2 = graph.GetUAV(Mip(pyramid, 2));
            constants.m_rwSpdMip3 = graph.GetUAV(Mip(pyramid, 3));
            constants.m_rwSpdMip4 = graph.GetUAV(Mip(pyramid, 4));
            constants.m_rwSpdMip5 = graph.GetUAV(Mip(pyramid, 5));
            constants.m_rwSpdGlobalAtomic = graph.GetUAV(frame.m_spdAtomic.Get());
            graph.AddDispatch("LumaPyramid",
                              LumaPyramidPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(frame.m_renderSize, kSpdTileSize));
        }


        void AddShadingChange(Core::FrameGraph& graph, const FrameResources& frame)
        {
            const History& history = *frame.m_history;
            Core::Texture* pyramid = frame.m_spdPyramid.Get();

            ShadingChangePyramidConstants pyramidConstants{};
            pyramidConstants.m_parameters = frame.m_parameters;
            pyramidConstants.m_linearClamp = frame.m_linearClamp;
            pyramidConstants.m_rCurrentLuma = graph.GetSRV(history.m_luma[frame.m_current].Get());
            pyramidConstants.m_rPreviousLuma = graph.GetSRV(history.m_luma[frame.m_previous].Get());
            pyramidConstants.m_rDilatedMotionVectors = graph.GetSRV(frame.m_dilatedMotion.Get());
            pyramidConstants.m_rwSpdMip0 = graph.GetUAV(Mip(pyramid, 0));
            pyramidConstants.m_rwSpdMip1 = graph.GetUAV(Mip(pyramid, 1));
            pyramidConstants.m_rwSpdMip2 = graph.GetUAV(Mip(pyramid, 2));
            pyramidConstants.m_rwSpdMip3 = graph.GetUAV(Mip(pyramid, 3));
            pyramidConstants.m_rwSpdMip4 = graph.GetUAV(Mip(pyramid, 4));
            pyramidConstants.m_rwSpdMip5 = graph.GetUAV(Mip(pyramid, 5));
            pyramidConstants.m_rwSpdGlobalAtomic = graph.GetUAV(frame.m_spdAtomic.Get());
            graph.AddDispatch("ShadingChangePyramid",
                              ShadingChangePyramidPipeline::GetPipeline(),
                              pyramidConstants,
                              Math::CeilDivide(frame.m_renderSize, kSpdTileSize));

            ShadingChangeConstants constants{};
            constants.m_parameters = frame.m_parameters;
            constants.m_linearClamp = frame.m_linearClamp;
            constants.m_rSpdMips = graph.GetSRV(pyramid);
            constants.m_rwShadingChange = graph.GetUAV(frame.m_shadingChange.Get());
            graph.AddDispatch("ShadingChange",
                              ShadingChangePipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(frame.m_halfSize, 8u));
        }


        void AddPrepareReactivity(Core::FrameGraph& graph, const FrameResources& frame)
        {
            const History& history = *frame.m_history;

            PrepareReactivityConstants constants{};
            constants.m_parameters = frame.m_parameters;
            constants.m_linearClamp = frame.m_linearClamp;
            constants.m_rReactiveMask = graph.GetSRV(frame.m_reactiveMask);
            constants.m_rTransparencyAndCompositionMask = graph.GetSRV(frame.m_compositionMask);
            constants.m_rAccumulation = graph.GetSRV(history.m_accumulation[frame.m_previous].Get());
            constants.m_rwAccumulation = graph.GetUAV(history.m_accumulation[frame.m_current].Get());
            constants.m_rShadingChange = graph.GetSRV(frame.m_shadingChange.Get());
            constants.m_rCurrentLuma = graph.GetSRV(history.m_luma[frame.m_current].Get());
            constants.m_rwNewLocks = graph.GetUAV(frame.m_newLocks.Get());
            constants.m_rReconstructedPreviousNearestDepth = graph.GetSRV(frame.m_reconstructedDepth.Get());
            constants.m_rDilatedMotionVectors = graph.GetSRV(frame.m_dilatedMotion.Get());
            constants.m_rDilatedDepth = graph.GetSRV(frame.m_dilatedDepth.Get());
            constants.m_rwDilatedReactiveMasks = graph.GetUAV(frame.m_reactiveMasks.Get());
            graph.AddDispatch("PrepareReactivity",
                              PrepareReactivityPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(frame.m_renderSize, 8u));
        }


        void AddLumaInstability(Core::FrameGraph& graph, const FrameResources& frame)
        {
            const History& history = *frame.m_history;

            LumaInstabilityConstants constants{};
            constants.m_parameters = frame.m_parameters;
            constants.m_linearClamp = frame.m_linearClamp;
            constants.m_rwLumaHistory = graph.GetUAV(history.m_lumaHistory[frame.m_current].Get());
            constants.m_rLumaHistory = graph.GetSRV(history.m_lumaHistory[frame.m_previous].Get());
            constants.m_rFarthestDepthMip1 = graph.GetSRV(frame.m_farthestDepthMip.Get());
            constants.m_rCurrentLuma = graph.GetSRV(history.m_luma[frame.m_current].Get());
            constants.m_rwLumaInstability = graph.GetUAV(frame.m_lumaInstability.Get());
            constants.m_rDilatedMotionVectors = graph.GetSRV(frame.m_dilatedMotion.Get());
            constants.m_rDilatedReactiveMasks = graph.GetSRV(frame.m_reactiveMasks.Get());
            constants.m_rFrameInfo = graph.GetSRV(history.m_frameInfo.Get());
            graph.AddDispatch("LumaInstability",
                              LumaInstabilityPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(frame.m_renderSize, 8u));
        }


        void AddAccumulate(Core::FrameGraph& graph, const FrameResources& frame)
        {
            const History& history = *frame.m_history;

            AccumulateConstants constants{};
            constants.m_parameters = frame.m_parameters;
            constants.m_linearClamp = frame.m_linearClamp;
            constants.m_rInputColorJittered = graph.GetSRV(frame.m_color);
            constants.m_rInternalUpscaledColor = graph.GetSRV(history.m_color[frame.m_previous].Get());
            constants.m_rwInternalUpscaledColor = graph.GetUAV(history.m_color[frame.m_current].Get());
            constants.m_rwUpscaledOutput = graph.GetUAV(frame.m_output.Get());
            constants.m_rFarthestDepthMip1 = graph.GetSRV(frame.m_farthestDepthMip.Get());
            constants.m_rCurrentLuma = graph.GetSRV(history.m_luma[frame.m_current].Get());
            constants.m_rLumaInstability = graph.GetSRV(frame.m_lumaInstability.Get());
            constants.m_rwNewLocks = graph.GetUAV(frame.m_newLocks.Get());
            constants.m_rDilatedMotionVectors = graph.GetSRV(frame.m_dilatedMotion.Get());
            constants.m_rDilatedReactiveMasks = graph.GetSRV(frame.m_reactiveMasks.Get());
            graph.AddDispatch("Accumulate",
                              AccumulatePipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(frame.m_outputSize, 8u));
        }
    } // namespace


    ViewModule::ViewModule(View* view)
        : ViewModuleBase(view)
        , m_history(festd::make_unique<History>())
    {
    }


    ViewModule::~ViewModule() = default;


    void ViewModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    void ViewModule::ResetHistory()
    {
        m_history->m_isValid = false;
    }


    void ViewModule::PrepareView(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard, ViewRenderSetup& setup)
    {
        History& history = *m_history;
        const Vector2UInt outputSize = setup.m_outputResolution;
        if (!CanUpscale(*m_view, outputSize))
        {
            ResetHistory();
            return;
        }

        // History is reprojected by exactly one frame, so a frame this view skipped invalidates it.
        const uint64_t fence = Renderer::Get().GetGraphicsQueue()->GetCurrentFence().m_value;
        if (fence != history.m_lastFence + 1)
            ResetHistory();

        const Vector2UInt renderSize = CalculateRenderSize(m_renderScale, outputSize);
        if (history.m_renderSize != renderSize || history.m_outputSize != outputSize)
        {
            ResetHistory();
            history.m_renderSize = renderSize;
            history.m_outputSize = outputSize;
            CreateHistoryTextures(graph, history);
        }

        const bool hasHistory = history.m_isValid;
        if (!hasHistory)
            history.m_frameIndex = 0;

        const uint32_t phaseCount = CalculateJitterPhaseCount(renderSize, outputSize);
        const Vector2 jitter = CalculateJitter(history.m_frameIndex, phaseCount);
        const Vector2 previousJitter = hasHistory ? history.m_previousJitter : jitter;

        const uint64_t ticks = Platform::GetTicks();
        float deltaTime = kDefaultDeltaTime;
        if (hasHistory)
        {
            const double elapsed = static_cast<double>(ticks - history.m_previousTicks) * Platform::GetSecondsPerTick();
            deltaTime = Math::Clamp(static_cast<float>(elapsed), 0.0f, 1.0f);
        }

        PassData& data = blackboard.Add<PassData>();
        data.m_history = &history;
        data.m_parameters = BuildParameters(*m_view, history, jitter, previousJitter, deltaTime);
        data.m_hasHistory = hasHistory;

        ViewData& camera = *setup.m_constants;
        if (hasHistory)
            camera.m_previousViewProjection = history.m_previousViewProjection;
        else
            camera.m_previousViewProjection = camera.m_unjitteredViewProjection;

        ApplyJitter(camera, jitter, renderSize, outputSize);

        // AddPasses revalidates the history once this frame is accumulated into it.
        history.m_isValid = false;
        history.m_previousViewProjection = camera.m_unjitteredViewProjection;
        history.m_previousJitter = jitter;
        history.m_previousTicks = ticks;
        history.m_lastFence = fence;

        setup.m_renderResolution = renderSize;
        setup.m_needsMotionVectors = true;
    }


    void ViewModule::Update(Core::FrameGraphBlackboard&) {}


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard)
    {
        const PassData* data = blackboard.TryGet<PassData>();
        if (data == nullptr)
            return;

        // Without its inputs the jittered scene color is presented as rendered, and the history stays invalid.
        const auto* surface = blackboard.TryGet<SurfaceEvaluation::Output>();
        if (surface == nullptr || surface->m_motion.Get() == nullptr)
            return;

        if (blackboard.Contains<ViewColorOverride>())
            return;

        FE_FG_SCOPE(graph, "Upscaler/FSR3");

        History& history = *data->m_history;
        RendererViewData& view = blackboard.Get<RendererViewData>();

        FrameResources frame = CreateFrameResources(graph, *data);
        frame.m_color = view.m_mainColorTarget;
        frame.m_depth = view.m_mainDepthTarget;
        frame.m_motion = surface->m_motion.Get();
        SelectMasks(blackboard, frame);

        ClearFrameResources(graph, frame);
        if (!data->m_hasHistory)
            ClearHistory(graph, history);

        AddPrepareInputs(graph, frame);
        AddLumaPyramid(graph, frame);
        AddShadingChange(graph, frame);
        AddPrepareReactivity(graph, frame);
        AddLumaInstability(graph, frame);
        AddAccumulate(graph, frame);

        Output& output = blackboard.Add<Output>();
        output.m_color = frame.m_output;
        view.m_outputColorTarget = output.m_color.Get();

        history.m_isValid = true;
        ++history.m_frameIndex;
    }
} // namespace FE::Graphics::Fsr3
