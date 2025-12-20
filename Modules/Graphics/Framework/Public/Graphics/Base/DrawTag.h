#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE::Graphics
{
    namespace Internal
    {
        uint32_t GetNextDrawTagValue();
    }


    struct DrawTag final : public TypedHandle<DrawTag, uint32_t>
    {
    };


    struct DrawTagMask final
    {
        DrawTagMask() = default;

        explicit DrawTagMask(const DrawTag tag)
            : m_value(UINT64_C(1) << tag.m_value)
        {
        }

        explicit DrawTagMask(const uint64_t mask)
            : m_value(mask)
        {
        }

        [[nodiscard]] bool Contains(const DrawTag tag) const
        {
            return (m_value & (UINT64_C(1) << tag.m_value)) != 0;
        }

        DrawTagMask& operator|=(const DrawTagMask other)
        {
            m_value |= other.m_value;
            return *this;
        }

        DrawTagMask& operator&=(const DrawTagMask other)
        {
            m_value &= other.m_value;
            return *this;
        }

        DrawTagMask operator|(const DrawTagMask other) const
        {
            return DrawTagMask(m_value | other.m_value);
        }

        DrawTagMask operator&(const DrawTagMask other) const
        {
            return DrawTagMask(m_value & other.m_value);
        }

        uint64_t m_value = 0;
    };


#define FE_DECLARE_DRAW_TAG(name)                                                                                                \
    namespace FE::Graphics::DrawTags                                                                                             \
    {                                                                                                                            \
        extern const DrawTag name;                                                                                               \
    }


#define FE_IMPLEMENT_DRAW_TAG(name)                                                                                              \
    const FE::Graphics::DrawTag FE::Graphics::DrawTags::name = FE::Graphics::Internal::GetNextDrawTagValue()
} // namespace FE::Graphics
