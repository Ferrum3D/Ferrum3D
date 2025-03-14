#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Graphics::Core
{
    enum class DrawArgumentsType : uint32_t
    {
        kIndexed,
        kLinear,
    };


    struct DrawArgumentsIndexed final
    {
        uint32_t m_vertexOffset;
        uint32_t m_indexOffset;
        uint32_t m_indexCount;
    };


    struct DrawArgumentsLinear final
    {
        uint32_t m_vertexOffset;
        uint32_t m_vertexCount;
    };


    struct DrawArguments final
    {
        DrawArgumentsType m_type;

        union
        {
            DrawArgumentsIndexed m_indexed;
            DrawArgumentsLinear m_linear;
        };

        void Init(const DrawArgumentsIndexed& args)
        {
            m_type = DrawArgumentsType::kIndexed;
            m_indexed = args;
        }

        void Init(const DrawArgumentsLinear& args)
        {
            m_type = DrawArgumentsType::kLinear;
            m_linear = args;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            switch (m_type)
            {
            case DrawArgumentsType::kIndexed:
                return DefaultHashWithSeed(festd::to_underlying(m_type), &m_indexed, sizeof(DrawArgumentsIndexed));

            case DrawArgumentsType::kLinear:
                return DefaultHashWithSeed(festd::to_underlying(m_type), &m_linear, sizeof(DrawArgumentsLinear));

            default:
                FE_DebugBreak();
                return 0;
            }
        }
    };
} // namespace FE::Graphics::Core
