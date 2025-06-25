#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Tools/Downsample.h>

#include <Shaders/Features/Tools/Downsample/Downsample.h>

namespace FE::Graphics::Tools
{
    namespace
    {
        struct Pipeline final : public Core::ComputePipelineVariantSet
        {
            FE_SHADER_SPEC_DEFINE_BOOL(AllowFloat16, "FFX_HALF");
            FE_SHADER_SPEC_DEFINE_ENUM(DownsampleFilter, "FFX_SPD_OPTION_DOWNSAMPLE_FILTER", Downsample::Filter);
            using Specializer = Core::ShaderSpecializer<AllowFloat16, DownsampleFilter>;

            FE_DECLARE_PIPELINE_SET(Pipeline, Specializer);

        private:
            void SetupRequest([[maybe_unused]] const uint32_t variantIndex, Core::ComputePipelineRequest& request) override
            {
                request.m_desc.SetComputeShader("Shaders/Features/Tools/Downsample/Downsample.cs.hlsl");
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(Pipeline);


        void SpdSetup(Vector2UInt& dispatchThreadGroupCount, Vector2UInt& workGroupOffset, uint32_t& numWorkGroups,
                      uint32_t& numMips, const RectUInt rect, const uint32_t mips)
        {
            // determines the offset of the first tile to downsample based on
            // left (rectInfo[0]) and top (rectInfo[1]) of the subregion.
            workGroupOffset = rect.min / 64u;

            const Vector2UInt endIndex = (rect.max - Vector2UInt(1)) / 64u;

            // we only need to dispatch as many thread groups as tiles we need to downsample
            // number of tiles per slice depends on the subregion to downsample
            dispatchThreadGroupCount = endIndex + Vector2UInt(1) - workGroupOffset;

            // number of thread groups per slice
            numWorkGroups = dispatchThreadGroupCount.x * dispatchThreadGroupCount.y;

            if (mips != kInvalidIndex)
            {
                numMips = mips;
            }
            else
            {
                // calculate based on rect width and height
                numMips = Math::Min(Downsample::kSpdMaxMipLevels, Core::CalculateMipCount(rect.Size()) - 1);
            }
        }

        static_assert(Downsample::kMaxMipCount == Downsample::kSpdMaxMipLevels, "Must be kept in sync");
    } // namespace


    festd::fixed_vector<Core::RenderTargetHandle, Downsample::kMaxMipCount> Downsample::AddPass(
        const Core::FrameGraphBuilder& builder, const Core::RenderTargetHandle src, const Settings settings)
    {
        const auto& graph = builder.GetGraph();

        const Core::ImageDesc sourceDesc = graph.GetResourceDesc(src);
        const RectUInt rect = RectUInt::FromPosAndSize({ 0, 0 }, sourceDesc.GetSize2D());

        Vector2UInt dispatchThreadGroupCount;
        Vector2UInt workGroupOffset;
        uint32_t numWorkGroups;
        uint32_t numMips;
        SpdSetup(dispatchThreadGroupCount, workGroupOffset, numWorkGroups, numMips, rect, settings.m_mipCount);

        const Core::FrameGraphPassBuilder pass = builder.AddPass("Downsample");
        const Env::Name sourceName = graph.GetResourceName(src);

        festd::fixed_vector<Core::RenderTargetHandle, kMaxMipCount> dst;
        dst.resize(numMips);

        const Core::RenderTargetHandle source = pass.Read(src, Core::ImageReadType::kShaderResource);

        for (uint32_t mipIndex = 1; mipIndex <= numMips; ++mipIndex)
        {
            const uint32_t width = Math::Max(1u, sourceDesc.m_width >> mipIndex);
            const uint32_t height = Math::Max(1u, sourceDesc.m_height >> mipIndex);

            const Env::Name mipName = Fmt::FormatName("{}_DownsampleMip_{}", sourceName, mipIndex);
            const Core::ImageDesc mipDesc = Core::ImageDesc::Img2D(width, height, sourceDesc.m_imageFormat, false);
            dst[mipIndex - 1] = pass.WriteUAV(pass.CreateImage(mipName, mipDesc));
        }

        const Core::BufferHandle globalAtomic =
            pass.Write(pass.CreateStructuredBuffer<SpdGlobalAtomicBuffer>("SpdGlobalAtomic", 1));

        pass.SetFunction([=](Core::FrameGraphContext& context) {
            Constants constants;
            constants.m_mips = numMips;
            constants.m_numWorkGroups = numWorkGroups;
            constants.m_workGroupOffset = workGroupOffset;
            constants.m_invInputSize = Math::Reciprocal(Vector2(sourceDesc.GetSize2D()));
            constants.m_internalGlobalAtomic = context.GetGraph().GetUAV(globalAtomic);

            constants.m_input = context.GetGraph().GetSRV(source);
            for (uint32_t mipIndex = 0; mipIndex < numMips; ++mipIndex)
                constants.m_inputSrcMips[mipIndex] = context.GetGraph().GetUAV(dst[mipIndex]);

            constants.m_inputSrcMidMip = constants.m_inputSrcMips[5];
            constants.m_linearClamp = context.GetGraph().GetSampler(Core::SamplerState::kLinearClamp);

            Pipeline::Specializer specializer;
            specializer.Set<Pipeline::AllowFloat16>(settings.m_allowFloat16);
            specializer.Set<Pipeline::DownsampleFilter>(settings.m_filter);

            context.PushConstants(constants);
            context.Dispatch(Pipeline::GetPipeline(specializer), dispatchThreadGroupCount);
        });

        return dst;
    }
} // namespace FE::Graphics::Tools
