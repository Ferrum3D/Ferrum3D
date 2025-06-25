#pragma once
#include <Graphics/Core/Base/Limits.h>
#include <Graphics/Core/ImageFormat.h>

namespace FE::Graphics::Core
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

        InputStreamLayout() = default;

        explicit InputStreamLayout(ForceInitType)
        {
            memset(this, 0, sizeof(*this));
        }

        void ResetStreams()
        {
            m_perInstanceStreamsMask = 0;
            m_activeChannelsMask = 0;
        }

        [[nodiscard]] uint32_t CalculateStreamStride(const uint32_t streamIndex) const
        {
            uint32_t stride = 0;
            Bit::Traverse(m_activeChannelsMask, [&](const uint32_t channelIndex) {
                if (m_channels[channelIndex].m_streamIndex == streamIndex)
                    stride += GetFormatSize(m_channels[channelIndex].m_format);
            });

            return stride;
        }

        [[nodiscard]] uint32_t CalculateTotalStride() const
        {
            uint32_t stride = 0;
            Bit::Traverse(m_activeChannelsMask, [&](const uint32_t channelIndex) {
                stride += GetFormatSize(m_channels[channelIndex].m_format);
            });

            return stride;
        }

        [[nodiscard]] uint32_t CalculateActiveStreamMask() const
        {
            uint32_t streamMask = 0;
            Bit::Traverse(m_activeChannelsMask, [&](const uint32_t channelIndex) {
                streamMask |= 1 << m_channels[channelIndex].m_streamIndex;
            });

            return streamMask;
        }

        [[nodiscard]] uint32_t CalculateActiveStreamCount() const
        {
            return Bit::PopCount(CalculateActiveStreamMask());
        }

        [[nodiscard]] uint32_t GetActiveChannelsCount() const
        {
            return Bit::PopCount(m_activeChannelsMask);
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
        }

        [[nodiscard]] bool IsEmpty() const
        {
            return m_activeChannelsMask == 0;
        }

        static const InputStreamLayout kNull;
    };

    inline const InputStreamLayout InputStreamLayout::kNull{ kForceInit };
} // namespace FE::Graphics::Core
