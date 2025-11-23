#pragma once
#include <FeCore/Math/Rect.h>

namespace FE::Graphics::Core
{
    struct Texture;


    namespace Internal
    {
        template<class T>
        struct PassDescParameterImpl
        {
            PassDescParameterImpl() = default;

            PassDescParameterImpl& operator=(const T value)
            {
                m_value = value;
                return *this;
            }

            T m_value;
        };
    } // namespace Internal


    template<class T>
    struct PassPushConstants final : public T
    {
    };


    struct PassColorTarget final : public Internal::PassDescParameterImpl<Texture*>
    {
    };


    struct PassDepthTarget final : public Internal::PassDescParameterImpl<Texture*>
    {
    };


    struct PassViewport final : public Internal::PassDescParameterImpl<RectF>
    {
    };


    struct PassScissor final : public Internal::PassDescParameterImpl<RectInt>
    {
    };
} // namespace FE::Graphics::Core
