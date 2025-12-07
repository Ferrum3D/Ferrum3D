#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
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


    festd::fixed_vector<Core::Texture*, Downsample::kMaxMipCount> Downsample::AddPass(Core::FrameGraph& graph, Core::Texture* src,
                                                                                      const Settings& settings)
    {
        const Core::TextureDesc sourceDesc = src->GetDesc();
        const Env::Name sourceName = src->GetName();
        const RectUInt rect = RectUInt::FromPosAndSize({ 0, 0 }, sourceDesc.GetSize2D());

        Vector2UInt dispatchThreadGroupCount;
        Vector2UInt workGroupOffset;
        uint32_t numWorkGroups;
        uint32_t numMips;
        SpdSetup(dispatchThreadGroupCount, workGroupOffset, numWorkGroups, numMips, rect, settings.m_mipCount);

        festd::fixed_vector<Core::Texture*, kMaxMipCount> dst;
        dst.resize(numMips);

        Core::ResourcePool* resourcePool = graph.GetResourcePool();
        for (uint32_t mipIndex = 1; mipIndex <= numMips; ++mipIndex)
        {
            const uint32_t width = Math::Max(1u, sourceDesc.m_width >> mipIndex);
            const uint32_t height = Math::Max(1u, sourceDesc.m_height >> mipIndex);

            const Env::Name mipName = Fmt::FormatName("{}_DownsampleMip_{}", sourceName, mipIndex);
            dst[mipIndex - 1] = resourcePool->CreateTexture(mipName, sourceDesc.m_imageFormat, { width, height });
        }

        Core::Buffer* globalAtomic = resourcePool->CreateStructuredBuffer<SpdGlobalAtomicBuffer>("SpdGlobalAtomic", 1);

        Pipeline::Specializer specializer;
        specializer.Set<Pipeline::AllowFloat16>(settings.m_allowFloat16);
        specializer.Set<Pipeline::DownsampleFilter>(settings.m_filter);

        PassDesc* passDesc = graph.AllocatePassData<PassDesc>();
        passDesc->m_pipeline = Pipeline::GetPipeline(specializer);

        Constants& constants = passDesc->m_constants;
        constants.m_mips = numMips;
        constants.m_numWorkGroups = numWorkGroups;
        constants.m_workGroupOffset = workGroupOffset;
        constants.m_invInputSize = Math::Reciprocal(Vector2(sourceDesc.GetSize2D()));
        constants.m_internalGlobalAtomic = graph.GetDescriptor(globalAtomic);

        constants.m_input = graph.GetDescriptor(src);
        for (uint32_t mipIndex = 0; mipIndex < numMips; ++mipIndex)
            constants.m_inputSrcMips[mipIndex] = graph.GetDescriptor(dst[mipIndex]);

        constants.m_inputSrcMidMip = constants.m_inputSrcMips[5];
        constants.m_linearClamp = graph.GetSampler(Core::SamplerState::kLinearClamp);

        graph.AddDispatchPass("Downsample", passDesc, dispatchThreadGroupCount);
        return dst;
    }
} // namespace FE::Graphics::Tools
