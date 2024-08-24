#pragma once
#include <HAL/InputStreamLayout.h>

namespace FE::Graphics::HAL
{
    class InputLayoutBufferBuilder final
    {
        friend class InputLayoutBuilder;

        InputLayoutBuilder* m_Parent = nullptr;
        InputStreamBufferDesc m_Buffer{};
        eastl::vector<InputStreamAttributeDesc> m_Attributes;

        uint32_t m_Index = 0;
        uint32_t m_Offset = 0;

    public:
        inline InputLayoutBufferBuilder& AddAttribute(Format format, StringSlice semantic)
        {
            m_Attributes.emplace_back(semantic, m_Index, m_Offset, format);
            m_Offset += GetFormatSize(format);
            return *this;
        }

        inline InputLayoutBufferBuilder& AddPadding(uint32_t bytes)
        {
            m_Offset += bytes;
            return *this;
        }

        inline InputLayoutBuilder& Build()
        {
            return *m_Parent;
        }
    };

    class InputLayoutBuilder final
    {
        PrimitiveTopology m_Topology;
        eastl::vector<InputLayoutBufferBuilder> m_Buffers;

    public:
        explicit InputLayoutBuilder(PrimitiveTopology topology = PrimitiveTopology::TriangleList)
        {
            m_Topology = topology;
        }

        inline InputLayoutBufferBuilder& AddBuffer(InputStreamRate inputRate)
        {
            auto& result = m_Buffers.emplace_back();
            result.m_Buffer.InputRate = inputRate;
            result.m_Index = static_cast<uint32_t>(m_Buffers.size() - 1);
            result.m_Parent = this;

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
} // namespace FE::Graphics::HAL
