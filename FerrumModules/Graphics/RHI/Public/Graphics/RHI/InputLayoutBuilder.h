#pragma once
#include <Graphics/RHI/InputStreamLayout.h>

namespace FE::Graphics::RHI
{
    struct InputLayoutBuilder;

    struct InputLayoutBufferBuilder final
    {
        InputLayoutBufferBuilder& AddAttribute(Format format, Env::Name semantic)
        {
            m_attributes.emplace_back(semantic, m_index, m_offset, format);
            m_offset += GetFormatSize(format);
            return *this;
        }

        InputLayoutBufferBuilder& AddPadding(uint32_t bytes)
        {
            m_offset += bytes;
            return *this;
        }

        InputLayoutBuilder& Build()
        {
            return *m_parent;
        }

    private:
        friend struct InputLayoutBuilder;

        InputLayoutBuilder* m_parent = nullptr;
        InputStreamBufferDesc m_buffer{};
        festd::vector<InputStreamAttributeDesc> m_attributes;

        uint32_t m_index = 0;
        uint32_t m_offset = 0;
    };


    struct InputLayoutBuilder final
    {
        explicit InputLayoutBuilder(PrimitiveTopology topology = PrimitiveTopology::kTriangleList)
        {
            m_topology = topology;
        }

        InputLayoutBufferBuilder& AddBuffer(InputStreamRate inputRate)
        {
            auto& result = m_buffers.emplace_back();
            result.m_buffer.m_inputRate = inputRate;
            result.m_index = static_cast<uint32_t>(m_buffers.size() - 1);
            result.m_parent = this;

            return result;
        }

        InputStreamLayout Build()
        {
            InputStreamLayout result;
            result.m_topology = m_topology;

            for (auto& bufferBuilder : m_buffers)
            {
                bufferBuilder.m_buffer.m_stride = bufferBuilder.m_offset;
                result.PushBuffer(bufferBuilder.m_buffer);

                for (auto& attribute : bufferBuilder.m_attributes)
                {
                    result.PushAttribute(attribute);
                }
            }

            return result;
        }

    private:
        PrimitiveTopology m_topology;
        festd::vector<InputLayoutBufferBuilder> m_buffers;
    };
} // namespace FE::Graphics::RHI
