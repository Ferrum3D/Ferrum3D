#pragma once
#include <Framework/Input/InputFrame.h>

namespace FE::GameFramework
{
    struct CameraMovementComponent final
    {
        FE_RTTI_Reflect("43E3DFBF-F155-4ACB-BE25-23857425062F");
        FE_RTTI_Serialize();

        Input::ActionReference<Vector2> m_moveAction;
        Input::ActionReference<float> m_elevateAction;
        Input::ActionReference<Vector2> m_lookAction;
        float m_moveSpeed = 4;
    };
} // namespace FE::GameFramework
