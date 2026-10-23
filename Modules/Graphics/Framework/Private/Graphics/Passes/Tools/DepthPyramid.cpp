#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>

#include <Shaders/Passes/Tools/DepthPyramid/DepthPyramid.h>

namespace FE::Graphics::Tools::DepthPyramid
{
    namespace
    {
        struct DepthPyramidTilePipeline final : public Core::ComputePipelineVariantSet
        {
            FE_SHADER_SPEC_CONSTANT_BOOL(WritesClosest);
            using Specializer = Core::ShaderSpecializer<WritesClosest>;

            FE_DECLARE_PIPELINE_SET(DepthPyramidTilePipeline, Specializer);

        private:
            void SetupRequest([[maybe_unused]] const uint32_t variantIndex, Core::ComputePipelineRequest& request) override
            {
                request.m_desc.SetComputeShader("Shaders/Passes/Tools/DepthPyramid/DepthPyramid.cs.hlsl");
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(DepthPyramidTilePipeline);


        struct DepthPyramidSpdPipeline final : public Core::ComputePipelineVariantSet
        {
            FE_SHADER_SPEC_CONSTANT_RANGE(MipCount, kSpdMaxMipCount + 1);
            FE_SHADER_SPEC_CONSTANT_BOOL(Farthest);
            using Specializer = Core::ShaderSpecializer<MipCount, Farthest>;

            FE_DECLARE_PIPELINE_SET(DepthPyramidSpdPipeline, Specializer);

        private:
            [[nodiscard]] bool IsVariantDiscarded(const uint32_t variantIndex) const override
            {
                return Specializer(variantIndex).Get<MipCount>() == 0;
            }

            void SetupRequest([[maybe_unused]] const uint32_t variantIndex, Core::ComputePipelineRequest& request) override
            {
                request.m_desc.SetComputeShader("Shaders/Passes/Tools/DepthPyramid/DepthPyramidSpd.cs.hlsl");
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(DepthPyramidSpdPipeline);


        // Texture descriptors store up to 15 mips. Every axis covers at least one tile group, so tile threads never fall
        // outside the pyramid.
        constexpr uint32_t kMaxSize = 1u << (kMaxMipCount - 1);
        constexpr uint32_t kMinSize = kTileGroupSize;


        Output CreatePyramid(Core::FrameGraph& graph, const Env::Name name, const Vector2UInt depthSize, const bool isPersistent)
        {
            const Vector2UInt paddedSize(Math::CeilPowerOfTwo(depthSize.x), Math::CeilPowerOfTwo(depthSize.y));

            Output output;
            output.m_size.x = Math::Clamp(paddedSize.x / 2, kMinSize, kMaxSize);
            output.m_size.y = Math::Clamp(paddedSize.y / 2, kMinSize, kMaxSize);
            output.m_uvScale = Vector2(depthSize) / Vector2(output.m_size * 2u);
            output.m_mipCount = Math::FloorLog2(Math::Max(output.m_size.x, output.m_size.y)) + 1;
            output.m_texture =
                Core::Texture::Create(graph.GetDevice(), name, Core::Format::kR32_SFLOAT, output.m_size, output.m_mipCount);
            if (isPersistent)
            {
                Core::ResourceCommitParams commitParams;
                commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
                commitParams.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
                graph.GetResourcePool()->CommitTextureMemory(output.m_texture.Get(), commitParams);
            }

            return output;
        }


        Core::TextureView GetMipView(const Output& pyramid, const uint32_t mipIndex)
        {
            const auto subresource = Core::TextureSubresource::Create(pyramid.m_texture->GetDesc(), mipIndex, 0);
            return Core::TextureView::Create(pyramid.m_texture.Get(), subresource);
        }


        void AddTilePass(Core::FrameGraph& graph, Core::Texture* depth, Core::Buffer* spdCounters, const Pyramids& pyramids,
                         const bool buildsClosest)
        {
            const Output& farthest = pyramids.m_farthest;
            FE_Assert(farthest.m_mipCount >= kTileMipCount);

            DepthPyramidTilePipeline::Specializer specializer;
            specializer.Set<DepthPyramidTilePipeline::WritesClosest>(buildsClosest);

            auto* desc = graph.AllocatePassData<TilePassDesc>();
            desc->m_pipeline = DepthPyramidTilePipeline::GetPipeline(specializer);

            TileConstants& constants = desc->m_constants;
            constants.m_depth = graph.GetSRV(depth);
            constants.m_sampler = graph.GetSampler(Core::SamplerState::kPointClamp);
            constants.m_invDepthSize = Math::Reciprocal(Vector2(depth->GetDesc().GetSize2D()));
            constants.m_spdCounters = graph.GetUAV(spdCounters);
            for (uint32_t mipIndex = 0; mipIndex < kTileMipCount; ++mipIndex)
            {
                constants.m_farthest[mipIndex] = graph.GetUAV(GetMipView(farthest, mipIndex));
                if (buildsClosest)
                    constants.m_closest[mipIndex] = graph.GetUAV(GetMipView(pyramids.m_closest, mipIndex));
            }

            graph.AddDispatchPass("BuildDepthPyramidTiles", desc, Math::CeilDivide(farthest.m_size, kTileGroupSize));
        }


        void AddSpdPass(Core::FrameGraph& graph, Core::Buffer* counters, const Output& pyramid, const bool isFarthest)
        {
            constexpr uint32_t kSourceMip = kTileMipCount - 1;
            const uint32_t mipCount = pyramid.m_mipCount - kTileMipCount;
            FE_Assert(mipCount <= kSpdMaxMipCount);

            const Vector2UInt sourceSize(Math::Max(pyramid.m_size.x >> kSourceMip, 1u),
                                         Math::Max(pyramid.m_size.y >> kSourceMip, 1u));
            const Vector2UInt workGroupCount = Math::CeilDivide(sourceSize, kSpdSourceTileSize);

            DepthPyramidSpdPipeline::Specializer specializer;
            specializer.Set<DepthPyramidSpdPipeline::MipCount>(mipCount);
            specializer.Set<DepthPyramidSpdPipeline::Farthest>(isFarthest);

            auto* desc = graph.AllocatePassData<SpdPassDesc>();
            desc->m_pipeline = DepthPyramidSpdPipeline::GetPipeline(specializer);

            SpdConstants& constants = desc->m_constants;
            constants.m_source = graph.GetSRV(GetMipView(pyramid, kSourceMip));
            constants.m_sampler = graph.GetSampler(Core::SamplerState::kPointClamp);
            constants.m_invSourceSize = Math::Reciprocal(Vector2(sourceSize));
            constants.m_sourceSize = sourceSize;
            constants.m_workGroupCount = workGroupCount.x * workGroupCount.y;
            constants.m_counterIndex = isFarthest ? 0 : 1;
            constants.m_atomicCounter = graph.GetUAV(counters);
            for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
                constants.m_mips[mipIndex] = graph.GetUAV(GetMipView(pyramid, kTileMipCount + mipIndex));

            // The last group reads the mid mip written by all others, so it is accessed through a coherent view.
            if (mipCount > kSpdMidMip)
                constants.m_midMip = constants.m_mips[kSpdMidMip];

            graph.AddDispatchPass(isFarthest ? "ReduceFarthestDepthPyramid" : "ReduceClosestDepthPyramid", desc, workGroupCount);
        }
    } // namespace


    Pyramids AddPass(Core::FrameGraph& graph, Core::Texture* depth, const Settings& settings)
    {
        FE_PROFILER_ZONE();
        FE_FG_SCOPE(graph, "DepthPyramid");

        const Vector2UInt depthSize = depth->GetDesc().GetSize2D();
        FE_Assert(depthSize.x > 0 && depthSize.y > 0);

        Pyramids pyramids;
        pyramids.m_farthest = CreatePyramid(graph, "FarthestDepthPyramid", depthSize, settings.m_isPersistent);
        if (settings.m_buildsClosest)
            pyramids.m_closest = CreatePyramid(graph, "ClosestDepthPyramid", depthSize, settings.m_isPersistent);

        // The tile pass clears the counters, so the frame-local buffer needs no upload.
        const auto spdCounters =
            Core::Buffer::CreateStructured<uint32_t>(graph.GetDevice(), "DepthPyramidCounters", kSpdCounterCount);
        AddTilePass(graph, depth, spdCounters.Get(), pyramids, settings.m_buildsClosest);
        if (pyramids.m_farthest.m_mipCount == kTileMipCount)
            return pyramids;

        AddSpdPass(graph, spdCounters.Get(), pyramids.m_farthest, true);
        if (settings.m_buildsClosest)
            AddSpdPass(graph, spdCounters.Get(), pyramids.m_closest, false);

        return pyramids;
    }
} // namespace FE::Graphics::Tools::DepthPyramid
