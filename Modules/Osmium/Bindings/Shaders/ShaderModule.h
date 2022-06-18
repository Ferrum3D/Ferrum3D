#pragma once
#include <GPU/Shader/IShaderModule.h>

namespace FE::GPU
{
    struct ShaderModuleDescBinding
    {
        const UInt8* ByteCode;
        size_t ByteCodeSize;
        const char* EntryPoint;
        Int32 Stage;
    };
}
