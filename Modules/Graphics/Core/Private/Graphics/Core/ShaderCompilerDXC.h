#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/ShaderCompiler.h>

#include <dxc/dxcapi.h>

namespace FE::Graphics::Core
{
    struct ShaderCompilerDXC final : public ShaderCompiler
    {
        FE_RTTI_Class(ShaderCompilerDXC, "9DAF49F9-4E5D-4042-B123-67200DC60A14");

        ShaderCompilerDXC(Logger* logger, IO::IStreamFactory* streamFactory);

        ShaderCompilerResult CompileShader(const ShaderCompilerArgs& args) override;

    private:
        LibraryLoader m_module;
        Logger* m_logger;
        IO::IStreamFactory* m_streamFactory;
        Rc<ShaderSourceCache> m_shaderSourceCache;

        Rc<IDxcUtils> m_dxcUtils;
        Rc<IDxcCompiler3> m_dxcCompiler;
        Rc<IDxcIncludeHandler> m_dxcIncludeHandler;
    };
} // namespace FE::Graphics::Core
