#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <HAL/ShaderCompiler.h>

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>

namespace FE::Graphics::HAL
{
    class ShaderCompilerDXC final : public ShaderCompiler
    {
        LibraryLoader m_Module;
        GraphicsAPI m_API;
        Logger* m_logger;

    public:
        FE_RTTI_Class(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        explicit ShaderCompilerDXC(Logger* logger, GraphicsAPI api);

        ByteBuffer CompileShader(const ShaderCompilerArgs& args) override;
    };
} // namespace FE::Graphics::HAL
