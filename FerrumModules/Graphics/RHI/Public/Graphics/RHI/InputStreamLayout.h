#pragma once
#include <Graphics/RHI/Base/Limits.h>
#include <Graphics/RHI/ImageFormat.h>

namespace FE::Graphics::RHI
{
    enum class PrimitiveTopology : uint32_t
    {
        kNone,
        kPointList,
        kLineList,
        kLineStrip,
        kTriangleList,
        kTriangleStrip,
    };


    enum class InputStreamRate : uint32_t
    {
        kPerVertex = 0,
        kPerInstance = 1,
    };


    struct InputStreamChannelDesc final
    {
        ShaderSemanticName m_shaderSemanticName : 4;
        uint32_t m_shaderSemanticIndex : 4;
        VertexChannelFormat m_format : 10;
        uint32_t m_streamIndex : 4;
        uint32_t m_offset : 10;
    };


    struct InputStreamLayout final
    {
        PrimitiveTopology m_topology : 5;
        uint32_t m_perInstanceStreamsMask : Limits::Pipeline::kMaxVertexStreams;
        uint32_t m_activeChannelsMask : Limits::Pipeline::kMaxStreamChannels;
        InputStreamChannelDesc m_channels[Limits::Pipeline::kMaxStreamChannels];

        void ResetStreams()
        {
            m_perInstanceStreamsMask = 0;
            m_activeChannelsMask = 0;
        }

        [[nodiscard]] uint32_t GetActiveChannelsCount() const
        {
            return Bit::PopCount(m_activeChannelsMask);
        }
    };
} // namespace FE::Graphics::RHI
