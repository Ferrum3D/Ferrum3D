#pragma once
#include <Graphics/Core/InputStreamLayout.h>

namespace FE::Graphics::Core
{
    struct InputLayoutBuilder;

    struct InputLayoutStreamBuilder final
    {
        InputLayoutStreamBuilder AddChannel(const Format format, const ShaderSemantic semantic)
        {
            AddChannelInternal(TranslateFormat(format), semantic, GetFormatSize(format));
            return *this;
        }

        InputLayoutStreamBuilder AddChannel(const VertexChannelFormat format, const ShaderSemantic semantic)
        {
            AddChannelInternal(format, semantic, GetFormatSize(TranslateFormat(format)));
            return *this;
        }

        InputLayoutStreamBuilder AddPadding(const uint32_t bytes)
        {
            m_offset += bytes;
            return *this;
        }

    private:
        friend InputLayoutBuilder;

        void AddChannelInternal(VertexChannelFormat format, ShaderSemantic semantic, uint32_t stride);

        InputLayoutBuilder* m_parent = nullptr;
        uint32_t m_index = 0;
        uint32_t m_offset = 0;
    };


    struct InputLayoutBuilder final
    {
        explicit InputLayoutBuilder(const PrimitiveTopology topology = PrimitiveTopology::kTriangleList)
        {
            m_layout.ResetStreams();
            m_layout.m_topology = topology;
        }

        void SetTopology(const PrimitiveTopology topology)
        {
            m_layout.m_topology = topology;
        }

        InputLayoutStreamBuilder AddStream(const InputStreamRate inputRate)
        {
            const uint32_t streamIndex = m_streamsCount++;
            if (inputRate == InputStreamRate::kPerInstance)
                m_layout.m_perInstanceStreamsMask = Bit::Set(m_layout.m_perInstanceStreamsMask, streamIndex);

            InputLayoutStreamBuilder streamBuilder;
            streamBuilder.m_parent = this;
            streamBuilder.m_index = streamIndex;
            return streamBuilder;
        }

        InputStreamLayout Build()
        {
            const InputStreamLayout result = m_layout;
            m_layout.ResetStreams();
            return result;
        }

    private:
        friend InputLayoutStreamBuilder;

        InputStreamChannelDesc& NewChannel()
        {
            const uint32_t channelIndex = m_channelsCount++;
            m_layout.m_activeChannelsMask = Bit::Set(m_layout.m_activeChannelsMask, channelIndex);
            return m_layout.m_channels[channelIndex];
        }

        InputStreamLayout m_layout = {};
        uint32_t m_channelsCount = 0;
        uint32_t m_streamsCount = 0;
    };


    inline void InputLayoutStreamBuilder::AddChannelInternal(const VertexChannelFormat format, const ShaderSemantic semantic,
                                                             const uint32_t stride)
    {
        InputStreamChannelDesc& channel = m_parent->NewChannel();
        channel.m_format = format;
        channel.m_shaderSemanticName = semantic.m_name;
        channel.m_shaderSemanticIndex = semantic.m_index;
        channel.m_offset = m_offset;
        channel.m_streamIndex = m_index;

        m_offset += stride;
    }
} // namespace FE::Graphics::Core
