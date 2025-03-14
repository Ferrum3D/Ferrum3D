#pragma once
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Math/Aabb.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/InputStreamLayout.h>
#include <Graphics/Core/PipelineStates.h>
#include <Graphics/Core/ShaderStage.h>

namespace FE::Graphics::Core
{
    struct ShaderResourceGroup;
    struct ShaderModule;


    enum class PipelineStatus : uint32_t
    {
        kNotReady,
        kReady,
        kError,
    };


    struct GraphicsPipelineDesc final
    {
        ShaderResourceGroup* m_shaderResourceGroups[Limits::Pipeline::kMaxShaderResourceGroups];
        Env::Name m_shaders[festd::to_underlying(ShaderStage::kGraphicsCount)];

        Format m_rtvFormats[Limits::Pipeline::kMaxColorAttachments];
        Format m_dsvFormat = Format::kUndefined;

        int32_t m_renderTargetCount : 4;
        int32_t m_sampleCount : 5;
        RasterizationState m_rasterization = RasterizationState::kFillNoCull;
        DepthStencilState m_depthStencil = DepthStencilState::kDisabled;
        ColorBlendState m_colorBlend = ColorBlendState::Create(TargetColorBlending::kDisabled);
        InputStreamLayout m_inputLayout = InputStreamLayout::kNull;

        GraphicsPipelineDesc()
        {
            memset(&m_shaderResourceGroups, 0, festd::size_bytes(m_shaderResourceGroups));
            memset(&m_shaders, 0xff, festd::size_bytes(m_shaders));
            memset(&m_rtvFormats, 0, festd::size_bytes(m_rtvFormats));

            m_renderTargetCount = 0;
            m_sampleCount = 1;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.UpdateRaw(DefaultHash(&m_shaderResourceGroups, festd::size_bytes(m_shaderResourceGroups)));

            for (const Env::Name shader : m_shaders)
                hasher.UpdateRaw(shader.GetHash());

            hasher.UpdateRaw(DefaultHash(&m_rtvFormats, m_renderTargetCount * sizeof(Format)));

            if (m_depthStencil.m_depthTestEnabled || m_depthStencil.m_stencilTestEnabled)
                hasher.Update(festd::to_underlying(m_dsvFormat));

            hasher.Update(m_renderTargetCount);
            hasher.Update(m_sampleCount);
            hasher.UpdateRaw(m_rasterization.GetHash());
            hasher.UpdateRaw(m_depthStencil.GetHash());
            hasher.UpdateRaw(m_colorBlend.GetHash(m_renderTargetCount));
            hasher.UpdateRaw(m_inputLayout.GetHash());
            return hasher.Finalize();
        }

        GraphicsPipelineDesc& SetVertexShader(const Env::Name shaderName)
        {
            m_shaders[festd::to_underlying(ShaderStage::kVertex)] = shaderName;
            return *this;
        }

        GraphicsPipelineDesc& SetPixelShader(const Env::Name shaderName)
        {
            m_shaders[festd::to_underlying(ShaderStage::kPixel)] = shaderName;
            return *this;
        }

        GraphicsPipelineDesc& SetRTVFormats(const festd::span<const Format> formats)
        {
            FE_Assert(formats.size() <= Limits::Pipeline::kMaxColorAttachments);
            std::copy(formats.begin(), formats.end(), m_rtvFormats);
            return *this;
        }

        GraphicsPipelineDesc& SetRTVFormat(const Format format)
        {
            FE_Assert(m_renderTargetCount == 0);
            m_renderTargetCount = 1;
            m_rtvFormats[0] = format;
            return *this;
        }

        GraphicsPipelineDesc& SetDSVFormat(const Format format)
        {
            m_dsvFormat = format;
            return *this;
        }

        GraphicsPipelineDesc& SetRasterization(const RasterizationState rasterization)
        {
            m_rasterization = rasterization;
            return *this;
        }

        GraphicsPipelineDesc& SetDepthStencil(const DepthStencilState depthStencil)
        {
            m_depthStencil = depthStencil;
            return *this;
        }

        GraphicsPipelineDesc& SetColorBlend(const ColorBlendState& colorBlend)
        {
            m_colorBlend = colorBlend;
            return *this;
        }

        GraphicsPipelineDesc& SetColorBlend(const TargetColorBlending colorBlend)
        {
            m_colorBlend = ColorBlendState::Create(colorBlend);
            return *this;
        }

        GraphicsPipelineDesc& SetInputLayout(const InputStreamLayout& inputLayout)
        {
            m_inputLayout = inputLayout;
            return *this;
        }
    };


    struct GraphicsPipeline : public DeviceObject
    {
        FE_RTTI_Class(GraphicsPipeline, "4EBE406C-C4D7-40E5-9485-91C18C8C2527");

        ~GraphicsPipeline() override = default;

        [[nodiscard]] const GraphicsPipelineDesc& GetDesc() const
        {
            return m_desc;
        }

        [[nodiscard]] PipelineStatus GetStatus() const
        {
            return m_status.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsReady() const
        {
            return m_status.load(std::memory_order_acquire) == PipelineStatus::kReady;
        }

        [[nodiscard]] WaitGroup* GetCompletionWaitGroup() const
        {
            return m_completionWaitGroup.Get();
        }

    protected:
        std::atomic<PipelineStatus> m_status = PipelineStatus::kNotReady;
        GraphicsPipelineDesc m_desc;
        Rc<WaitGroup> m_completionWaitGroup;
    };
} // namespace FE::Graphics::Core
