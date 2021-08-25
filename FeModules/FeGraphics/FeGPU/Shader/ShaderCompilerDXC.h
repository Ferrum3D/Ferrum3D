#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <FeGPU/Shader/IShaderCompiler.h>
#include <FeCore/Utils/Interlocked.h>

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>

namespace FE::GPU
{
    class ShaderCompilerDXC : public Object<IShaderCompiler>
    {
        DynamicLibrary m_Module;

    public:
        FE_CLASS_RTTI(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        ShaderCompilerDXC();

        Vector<UInt8> CompileShader(const ShaderCompilerArgs& args) override;
    };
} // namespace FE::GPU
