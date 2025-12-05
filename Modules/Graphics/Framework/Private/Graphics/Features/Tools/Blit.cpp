#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Core/InputLayoutBuilder.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Features/Tools/Blit.h>

#include <Shaders/Features/Tools/Blit/Blit.h>

namespace FE::Graphics::Tools
{
    namespace
    {
        struct Pipeline final : public Core::GraphicsPipelineVariantSet
        {
            FE_SHADER_SPEC(ColorTargetFormat, Core::Format::kR8G8B8A8_UNORM, Core::Format::kB8G8R8A8_UNORM,
                           Core::Format::kB10G11R11_UFLOAT);
            using Specializer = Core::ShaderSpecializer<ColorTargetFormat>;

            FE_DECLARE_PIPELINE_SET(Pipeline, Specializer);

        private:
            void SetupRequest(const uint32_t variantIndex, Core::GraphicsPipelineRequest& request) override
            {
                const Specializer specializer(variantIndex);

                request.m_desc.SetTopology(Core::PrimitiveTopology::kTriangleList)
                    .SetPixelShader("Shaders/Features/Tools/Blit/Blit.ps.hlsl")
                    .SetVertexShader("Shaders/Features/Tools/Blit/Blit.vs.hlsl")
                    .SetRTVFormat(specializer.Get<ColorTargetFormat>());
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(Pipeline);
    } // namespace


    void Blit::AddPass(Core::FrameGraph& graph, Core::Texture* src, Core::Texture* dst, const Settings settings)
    {
        const Core::Format colorTargetFormat = dst->GetDesc().m_imageFormat;

        Pipeline::Specializer specializer;
        specializer.Set<Pipeline::ColorTargetFormat>(colorTargetFormat);

        auto* passDesc = graph.AllocatePassData<PassDesc>();
        passDesc->m_colorTarget = dst;
        passDesc->m_pipeline = Pipeline::GetPipeline(specializer);
        passDesc->m_constants.m_input = graph.GetDescriptor(src);
        passDesc->m_constants.m_sampler = graph.GetSampler(Core::SamplerState::kLinearWrap);
        passDesc->m_constants.m_uvScale = settings.m_destinationRect.Size();
        passDesc->m_constants.m_uvOffset = settings.m_destinationRect.min;
        graph.AddDrawIndexedInstancedPass("Blit", passDesc, 6);
    }
} // namespace FE::Graphics::Tools
