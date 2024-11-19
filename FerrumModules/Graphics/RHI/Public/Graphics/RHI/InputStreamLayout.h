#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/Strings/FixedString.h>
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
        kNone,
        kPerVertex,
        kPerInstance,
    };


    struct InputStreamBufferDesc final
    {
        bool operator==(const InputStreamBufferDesc& rhs) const
        {
            return m_stride == rhs.m_stride && m_inputRate == rhs.m_inputRate;
        }

        bool operator!=(const InputStreamBufferDesc& rhs) const
        {
            return !(rhs == *this);
        }

        uint32_t m_stride;
        InputStreamRate m_inputRate;
    };


    struct InputStreamAttributeDesc final
    {
        InputStreamAttributeDesc() = default;

        InputStreamAttributeDesc(Env::Name shaderSemantic, uint32_t bufferIndex, uint32_t offset, Format elementFormat)
            : m_shaderSemantic(shaderSemantic)
            , m_bufferIndex(bufferIndex)
            , m_offset(offset)
            , m_elementFormat(elementFormat)
        {
        }

        bool operator==(const InputStreamAttributeDesc& rhs) const
        {
            return m_shaderSemantic == rhs.m_shaderSemantic && m_bufferIndex == rhs.m_bufferIndex && m_offset == rhs.m_offset
                && m_elementFormat == rhs.m_elementFormat;
        }

        bool operator!=(const InputStreamAttributeDesc& rhs) const
        {
            return !(rhs == *this);
        }

        Env::Name m_shaderSemantic;
        uint32_t m_bufferIndex = 0;
        uint32_t m_offset = 0;
        Format m_elementFormat = Format::kUndefined;
    };


    struct InputStreamLayout final
    {
        void PushBuffer(const InputStreamBufferDesc& bufferDesc)
        {
            m_buffers.push_back(bufferDesc);
        }

        void PushAttribute(const InputStreamAttributeDesc& attributeDesc)
        {
            m_attributes.push_back(attributeDesc);
        }

        bool operator==(const InputStreamLayout& rhs) const
        {
            if (m_buffers.size() != rhs.m_buffers.size())
                return false;

            if (m_attributes.size() != rhs.m_attributes.size())
                return false;

            for (uint32_t i = 0; i < m_buffers.size(); ++i)
            {
                if (m_buffers[i] != rhs.m_buffers[i])
                    return false;
            }

            for (uint32_t i = 0; i < m_attributes.size(); ++i)
            {
                if (m_attributes[i] != rhs.m_attributes[i])
                    return false;
            }

            return m_topology == rhs.m_topology;
        }

        bool operator!=(const InputStreamLayout& rhs) const
        {
            return !(rhs == *this);
        }

        PrimitiveTopology m_topology;
        festd::vector<InputStreamBufferDesc> m_buffers;
        festd::vector<InputStreamAttributeDesc> m_attributes;
    };
} // namespace FE::Graphics::RHI
