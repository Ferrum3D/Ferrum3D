#pragma once
#include <OsGPU/Shader/IShaderReflection.h>
#include <spirv_hlsl.hpp>

namespace FE::GPU
{
    class VKShaderReflection : public Object<IShaderReflection>
    {
        Vector<ShaderInputAttribute> m_InputAttributes;

        void ParseInputAttributes(spirv_cross::CompilerHLSL& compiler);

    public:
        FE_CLASS_RTTI(VKShaderReflection, "686E6EBE-8038-4E26-919C-70834410BC1F");

        explicit VKShaderReflection(const Vector<UInt32>& byteCode);
        Vector<ShaderInputAttribute> GetInputAttributes() override;
        UInt32 GetInputAttributeLocation(StringSlice semantic) override;
    };
}
