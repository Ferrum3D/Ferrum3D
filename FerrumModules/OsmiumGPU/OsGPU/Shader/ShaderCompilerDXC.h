#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <FeCore/Parallel/Interlocked.h>
#include <OsGPU/Shader/IShaderCompiler.h>

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>

namespace FE::Osmium
{
    class ShaderCompilerDXC : public Object<IShaderCompiler>
    {
        DynamicLibrary m_Module;
        GraphicsAPI m_API;

    public:
        FE_CLASS_RTTI(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        explicit ShaderCompilerDXC(GraphicsAPI api);

        Shared<IByteBuffer> CompileShader(const ShaderCompilerArgs& args) override;
    };
} // namespace FE::Osmium
