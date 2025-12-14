#pragma once
#include <Graphics/Scene/View.h>

namespace FE::Graphics
{
    struct ViewImpl final : public View
    {
        FE_RTTI("113AC827-90AC-48F7-9A7A-4C7AE0695AD8");

        explicit ViewImpl(Scene* scene);
        ~ViewImpl() override;
    };
} // namespace FE::Graphics
