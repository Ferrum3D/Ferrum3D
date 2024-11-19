#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <Graphics/RHI/ShaderCompiler.h>

#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/dxcapi.h>

namespace FE::Graphics::RHI
{
    struct ShaderCompilerDXC final : public ShaderCompiler
    {
        FE_RTTI_Class(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        explicit ShaderCompilerDXC(Logger* logger, GraphicsAPI api);

        ByteBuffer CompileShader(const ShaderCompilerArgs& args) override;

    private:
        LibraryLoader m_module;
        GraphicsAPI m_api;
        Logger* m_logger;
    };
} // namespace FE::Graphics::RHI
