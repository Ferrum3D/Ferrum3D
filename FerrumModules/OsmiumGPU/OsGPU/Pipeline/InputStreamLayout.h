#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <OsGPU/Image/ImageFormat.h>

namespace FE::Osmium
{
    enum class PrimitiveTopology
    {
        None,
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip
    };

    enum class InputStreamRate
    {
        None,
        PerVertex,
        PerInstance
    };

    struct InputStreamBufferDesc
    {
        FE_RTTI_Base(InputStreamBufferDesc, "EBFE9878-7F92-4B91-9DDF-A879C9180DEC");

        bool operator==(const InputStreamBufferDesc& rhs) const
        {
            return Stride == rhs.Stride && InputRate == rhs.InputRate;
        }

        bool operator!=(const InputStreamBufferDesc& rhs) const
        {
            return !(rhs == *this);
        }

        uint32_t Stride;
        InputStreamRate InputRate;
    };

    struct InputStreamAttributeDesc
    {
        FE_RTTI_Base(InputStreamAttributeDesc, "5D1A2410-3399-4C49-A7DC-0E7BA680C49C");

        InputStreamAttributeDesc() = default;

        inline InputStreamAttributeDesc(const String& shaderSemantic, uint32_t bufferIndex, uint32_t offset, Format elementFormat)
            : ShaderSemantic(shaderSemantic)
            , BufferIndex(bufferIndex)
            , Offset(offset)
            , ElementFormat(elementFormat)
        {
        }

        bool operator==(const InputStreamAttributeDesc& rhs) const
        {
            return ShaderSemantic == rhs.ShaderSemantic && BufferIndex == rhs.BufferIndex && Offset == rhs.Offset
                && ElementFormat == rhs.ElementFormat;
        }

        bool operator!=(const InputStreamAttributeDesc& rhs) const
        {
            return !(rhs == *this);
        }

        FE::String ShaderSemantic;
        uint32_t BufferIndex = 0;
        uint32_t Offset = 0;
        Format ElementFormat = Format::None;
    };

    class InputStreamLayout
    {
        eastl::vector<InputStreamBufferDesc> m_Buffers;
        eastl::vector<InputStreamAttributeDesc> m_Attributes;

    public:
        inline void PushBuffer(const InputStreamBufferDesc& bufferDesc)
        {
            m_Buffers.push_back(bufferDesc);
        }

        inline void PushAttribute(const InputStreamAttributeDesc& attributeDesc)
        {
            m_Attributes.push_back(attributeDesc);
        }

        inline eastl::vector<InputStreamBufferDesc>& GetBuffers()
        {
            return m_Buffers;
        }

        inline eastl::vector<InputStreamAttributeDesc>& GetAttributes()
        {
            return m_Attributes;
        }

        [[nodiscard]] inline const eastl::vector<InputStreamBufferDesc>& GetBuffers() const
        {
            return m_Buffers;
        }

        [[nodiscard]] inline const eastl::vector<InputStreamAttributeDesc>& GetAttributes() const
        {
            return m_Attributes;
        }

        bool operator==(const InputStreamLayout& rhs) const
        {
            if (m_Buffers.size() != rhs.m_Buffers.size() || m_Attributes.size() != rhs.m_Attributes.size())
            {
                return false;
            }

            for (uint32_t i = 0; i < m_Buffers.size(); ++i)
            {
                if (m_Buffers[i] != rhs.m_Buffers[i])
                    return false;
            }

            for (uint32_t i = 0; i < m_Attributes.size(); ++i)
            {
                if (m_Attributes[i] != rhs.m_Attributes[i])
                    return false;
            }

            return Topology == rhs.Topology;
        }

        bool operator!=(const InputStreamLayout& rhs) const
        {
            return !(rhs == *this);
        }

        PrimitiveTopology Topology;
    };
} // namespace FE::Osmium
