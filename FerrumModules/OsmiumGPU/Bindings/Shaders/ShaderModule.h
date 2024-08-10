#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Osmium
{
    struct ShaderModuleDescBinding
    {
        const uint8_t* ByteCode;
        size_t ByteCodeSize;
        const char* EntryPoint;
        int32_t Stage;
    };
} // namespace FE::Osmium
