#include <FeCore/Containers/IByteBuffer.h>
#include <OsGPU/Shader/IShaderCompiler.h>

namespace FE::Osmium
{
    extern "C"
    {
        struct ShaderCompilerArgsBinding
        {
            HLSLShaderVersion Version;
            ShaderStage Stage;
            const char* SourceCode;
            const char* EntryPoint;
            const char* FullPath;
        };

        FE_DLL_EXPORT IByteBuffer* IShaderCompiler_CompileShader(IShaderCompiler* self, ShaderCompilerArgsBinding* args)
        {
            ShaderCompilerArgs a;
            a.Version    = args->Version;
            a.Stage      = args->Stage;
            a.SourceCode = args->SourceCode;
            a.EntryPoint = args->EntryPoint;
            a.FullPath   = args->FullPath;

            return self->CompileShader(a).Detach();
        }

        FE_DLL_EXPORT void IShaderCompiler_Destruct(IShaderCompiler* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium