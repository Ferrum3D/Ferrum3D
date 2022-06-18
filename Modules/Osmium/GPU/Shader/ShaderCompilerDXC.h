#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <FeCore/Parallel/Interlocked.h>
#include <GPU/Shader/IShaderCompiler.h>

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>

namespace FE::GPU
{
    class ShaderCompilerDXC : public Object<IShaderCompiler>
    {
        DynamicLibrary m_Module;
        GraphicsAPI m_API;

    public:
        FE_CLASS_RTTI(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        ShaderCompilerDXC(GraphicsAPI api);

        Shared<IByteBuffer> CompileShader(const ShaderCompilerArgs& args) override;
    };
} // namespace FE::GPU
