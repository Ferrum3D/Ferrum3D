#pragma once
#include <Framework/Input/MappingTable.h>

namespace FE::GameFramework
{
    struct CameraMovementComponent final
    {
        FE_RTTI_Reflect("43E3DFBF-F155-4ACB-BE25-23857425062F");
        FE_RTTI_Serialize();

        IO::Link<Input::Action> m_moveAction;
        IO::Link<Input::Action> m_elevateAction;
        IO::Link<Input::Action> m_lookAction;
        float m_moveSpeed = 4;
    };
} // namespace FE::GameFramework
