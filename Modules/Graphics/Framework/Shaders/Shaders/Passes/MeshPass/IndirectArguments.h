#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)

    struct MeshDispatchArguments final
    {
        uint32_t m_groupCountX;
        uint32_t m_groupCountY;
        uint32_t m_groupCountZ;
    };

FE_HOST_END_NAMESPACE
