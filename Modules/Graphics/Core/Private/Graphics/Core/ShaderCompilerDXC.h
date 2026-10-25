#pragma once
#include <Core/Base/PlatformInclude.h>
#include <Core/Env/LibraryLoader.h>
#include <Graphics/Core/Common/ShaderCache.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/ShaderCompiler.h>

#include <dxc/dxcapi.h>

namespace FE::Graphics::Core
{
    struct ShaderCompilerDXC final : public ShaderCompiler
    {
        FE_RTTI("9DAF49F9-4E5D-4042-B123-67200DC60A14");

        ShaderCompilerDXC();

        ShaderCompilerResult CompileShader(const ShaderCompilerArgs& args) override;

        [[nodiscard]] ShaderSourceCache* GetSourceCache() const
        {
            return m_shaderSourceCache.Get();
        }

    private:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        LibraryLoader m_module;
        Rc<ShaderSourceCache> m_shaderSourceCache;
        ShaderCache m_shaderCache;
        uint64_t m_compilerVersionHash = 0;

        Rc<IDxcUtils> m_dxcUtils;
        Rc<IDxcCompiler3> m_dxcCompiler;
    };
} // namespace FE::Graphics::Core
