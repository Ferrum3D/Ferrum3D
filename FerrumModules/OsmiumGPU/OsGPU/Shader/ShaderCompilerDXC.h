#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <OsGPU/Shader/IShaderCompiler.h>

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>

namespace FE::Osmium
{
    class ShaderCompilerDXC : public IShaderCompiler
    {
        LibraryLoader m_Module;
        GraphicsAPI m_API;

    public:
        FE_RTTI_Class(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        explicit ShaderCompilerDXC(GraphicsAPI api);

        ByteBuffer CompileShader(const ShaderCompilerArgs& args) override;
    };
} // namespace FE::Osmium
