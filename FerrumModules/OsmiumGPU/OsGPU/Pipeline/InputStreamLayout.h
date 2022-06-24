#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/Containers/List.h>
#include <OsGPU/Image/ImageFormat.h>

namespace FE::GPU
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
        FE_STRUCT_RTTI(InputStreamBufferDesc, "EBFE9878-7F92-4B91-9DDF-A879C9180DEC");

        bool operator==(const InputStreamBufferDesc& rhs) const
        {
            return Stride == rhs.Stride && InputRate == rhs.InputRate;
        }

        bool operator!=(const InputStreamBufferDesc& rhs) const
        {
            return !(rhs == *this);
        }

        UInt32 Stride;
        InputStreamRate InputRate;
    };

    struct InputStreamAttributeDesc
    {
        FE_STRUCT_RTTI(InputStreamAttributeDesc, "5D1A2410-3399-4C49-A7DC-0E7BA680C49C");

        InputStreamAttributeDesc() = default;

        inline InputStreamAttributeDesc(const String& shaderSemantic, UInt32 bufferIndex, UInt32 offset, Format elementFormat)
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
        UInt32 BufferIndex   = 0;
        UInt32 Offset        = 0;
        Format ElementFormat = Format::None;
    };

    class InputStreamLayout
    {
        List<InputStreamBufferDesc> m_Buffers;
        List<InputStreamAttributeDesc> m_Attributes;

    public:
        inline void PushBuffer(const InputStreamBufferDesc& bufferDesc)
        {
            m_Buffers.Push(bufferDesc);
        }

        inline void PushAttribute(const InputStreamAttributeDesc& attributeDesc)
        {
            m_Attributes.Push(attributeDesc);
        }

        inline List<InputStreamBufferDesc>& GetBuffers()
        {
            return m_Buffers;
        }

        inline List<InputStreamAttributeDesc>& GetAttributes()
        {
            return m_Attributes;
        }

        inline const List<InputStreamBufferDesc>& GetBuffers() const
        {
            return m_Buffers;
        }

        inline const List<InputStreamAttributeDesc>& GetAttributes() const
        {
            return m_Attributes;
        }

        bool operator==(const InputStreamLayout& rhs) const
        {
            if (m_Buffers.Size() != rhs.m_Buffers.Size() || m_Attributes.Size() != rhs.m_Attributes.Size())
            {
                return false;
            }

            for (size_t i = 0; i < m_Buffers.Size(); ++i)
            {
                if (m_Buffers[i] != rhs.m_Buffers[i])
                {
                    return false;
                }
            }

            for (size_t i = 0; i < m_Attributes.Size(); ++i)
            {
                if (m_Attributes[i] != rhs.m_Attributes[i])
                {
                    return false;
                }
            }

            return Topology == rhs.Topology;
        }

        bool operator!=(const InputStreamLayout& rhs) const
        {
            return !(rhs == *this);
        }

        PrimitiveTopology Topology;
    };
} // namespace FE::GPU
