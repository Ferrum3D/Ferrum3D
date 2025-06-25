#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>
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

                Core::InputLayoutBuilder layoutBuilder;
                layoutBuilder.SetTopology(Core::PrimitiveTopology::kTriangleList);

                request.m_desc.SetInputLayout(layoutBuilder.Build())
                    .SetPixelShader("Shaders/Features/Tools/Blit/Blit.ps.hlsl")
                    .SetVertexShader("Shaders/Features/Tools/Blit/Blit.vs.hlsl")
                    .SetRTVFormat(specializer.Get<ColorTargetFormat>())
                    .SetColorBlend(Core::TargetColorBlending::kDisabled)
                    .SetDepthStencil(Core::DepthStencilState::kDisabled)
                    .SetRasterization(Core::RasterizationState::kFillNoCull);
            }
        };
        FE_IMPLEMENT_PIPELINE_SET(Pipeline);
    } // namespace


    Core::RenderTargetHandle Blit::AddPass(Core::FrameGraphBuilder& builder, const Core::RenderTargetHandle src,
                                           const Core::RenderTargetHandle dst, const Settings settings)
    {
        const auto& graph = builder.GetGraph();

        const auto pass = builder.AddPass("Blit");
        const auto source = pass.Read(src, Core::ImageReadType::kShaderResource);
        const auto destination = pass.WriteRenderTarget(dst);

        const Core::ImageDesc dstDesc = graph.GetResourceDesc(dst);
        const Core::Format colorTargetFormat = dstDesc.m_imageFormat;

        pass.SetFunction([=](Core::FrameGraphContext& context) {
            Constants constants;
            constants.m_input = context.GetGraph().GetSRV(source);
            constants.m_sampler = context.GetGraph().GetSampler(Core::SamplerState::kLinearWrap);
            constants.m_uvScale = settings.m_destinationRect.Size();
            constants.m_uvOffset = settings.m_destinationRect.min;
            context.PushConstants(constants);

            context.SetRenderTarget(destination);
            context.SetViewport(dstDesc.GetSize2D());

            if (Math::EqualEstimate(settings.m_destinationRect, RectF{ 0.0f, 0.0f, 1.0f, 1.0f }))
                context.SetRenderTargetLoadOperations(Core::RenderTargetLoadOperations{}.DiscardAll());
            else
                context.SetRenderTargetLoadOperations(Core::RenderTargetLoadOperations::kDefault);

            context.SetRenderTargetStoreOperations(Core::RenderTargetStoreOperations::kDefault);

            Pipeline::Specializer specializer;
            specializer.Set<Pipeline::ColorTargetFormat>(colorTargetFormat);
            context.Draw(Pipeline::GetPipeline(specializer), 6);
        });

        return destination;
    }
} // namespace FE::Graphics::Tools
