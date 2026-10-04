#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/FrameGraph/FrameGraphPass.h>
#include <Graphics/Core/InputLayoutBuilder.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/Tools/Tonemap.h>

#include <Shaders/Passes/Tools/Blit/Blit.h>

namespace FE::Graphics::Tools::Tonemap
{
    namespace
    {
        struct Pipeline final : public Core::GraphicsPipelineVariantSet
        {
            FE_SHADER_SPEC(ColorTargetFormat, Core::Format::kR8G8B8A8_UNORM, Core::Format::kB8G8R8A8_UNORM);
            using Specializer = Core::ShaderSpecializer<ColorTargetFormat>;

            FE_DECLARE_PIPELINE_SET(Pipeline, Specializer);

        private:
            void SetupRequest(const uint32_t variantIndex, Core::GraphicsPipelineRequest& request) override
            {
                const Specializer specializer(variantIndex);

                request.m_desc.SetTopology(Core::PrimitiveTopology::kTriangleList)
                    .SetPixelShader("Shaders/Passes/Tools/Tonemap/Tonemap.ps.hlsl")
                    .SetVertexShader("Shaders/Passes/Tools/Blit/Blit.vs.hlsl")
                    .SetRTVFormat(specializer.Get<ColorTargetFormat>());
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(Pipeline);
    } // namespace


    void AddPass(Core::FrameGraph& graph, const Core::TextureView src, const Core::TextureView dst)
    {
        const Core::Format colorTargetFormat = dst.GetBaseDesc().m_imageFormat;

        Pipeline::Specializer specializer;
        specializer.Set<Pipeline::ColorTargetFormat>(colorTargetFormat);

        auto* passDesc = graph.AllocatePassData<Blit::PassDesc>();
        passDesc->m_colorTarget = dst;
        passDesc->m_pipeline = Pipeline::GetPipeline(specializer);
        passDesc->m_constants.m_input = graph.GetSRV(src);
        passDesc->m_constants.m_sampler = graph.GetSampler(Core::SamplerState::kLinearClamp);
        passDesc->m_constants.m_uvScale = Vector2(1.0f, 1.0f);
        passDesc->m_constants.m_uvOffset = Vector2(0.0f, 0.0f);
        graph.AddDrawPass("Tonemap", passDesc, 6);
    }
} // namespace FE::Graphics::Tools::Tonemap
