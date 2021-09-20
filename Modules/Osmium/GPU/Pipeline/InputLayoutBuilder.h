#pragma once
#include <GPU/Pipeline/InputStreamLayout.h>

namespace FE::GPU
{
    class InputLayoutBufferBuilder
    {
        friend class InputLayoutBuilder;

        InputLayoutBuilder* m_Parent = nullptr;
        InputStreamBufferDesc m_Buffer{};
        Vector<InputStreamAttributeDesc> m_Attributes;

        UInt32 m_Index  = 0;
        UInt32 m_Offset = 0;

    public:
        inline InputLayoutBufferBuilder& AddAttribute(Format format, const FE::String& semantic)
        {
            m_Attributes.emplace_back(semantic, m_Index, m_Offset, format);
            m_Offset += GetFormatSize(format);
            return *this;
        }

        inline InputLayoutBufferBuilder& AddPadding(UInt32 bytes)
        {
            m_Offset += bytes;
            return *this;
        }

        inline InputLayoutBuilder& Build()
        {
            return *m_Parent;
        }
    };

    class InputLayoutBuilder
    {
        PrimitiveTopology m_Topology;
        Vector<InputLayoutBufferBuilder> m_Buffers;

    public:
        explicit InputLayoutBuilder(PrimitiveTopology topology = PrimitiveTopology::TriangleList)
        {
            m_Topology = topology;
        }

        inline InputLayoutBufferBuilder& AddBuffer(InputStreamRate inputRate)
        {
            auto& result              = m_Buffers.emplace_back();
            result.m_Buffer.InputRate = inputRate;
            result.m_Index            = static_cast<UInt32>(m_Buffers.size() - 1);
            result.m_Parent           = this;

            return result;
        }

        inline InputStreamLayout Build()
        {
            InputStreamLayout result;
            result.Topology = m_Topology;

            for (auto& bufferBuilder : m_Buffers)
            {
                bufferBuilder.m_Buffer.Stride = bufferBuilder.m_Offset;
                result.PushBuffer(bufferBuilder.m_Buffer);

                for (auto& attribute : bufferBuilder.m_Attributes)
                {
                    result.PushAttribute(attribute);
                }
            }

            return result;
        }
    };
} // namespace FE::GPU
