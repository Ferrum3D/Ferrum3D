#pragma once
#include <FeCore/Base/Base.h>

namespace FE::GPU
{
    struct ShaderModuleDescBinding
    {
        const UInt8* ByteCode;
        USize ByteCodeSize;
        const char* EntryPoint;
        Int32 Stage;
    };
} // namespace FE::GPU
