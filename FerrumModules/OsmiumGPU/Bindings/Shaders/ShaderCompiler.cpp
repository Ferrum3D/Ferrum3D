#include <FeCore/Containers/ByteBuffer.h>
#include <OsGPU/Shader/IShaderCompiler.h>

namespace FE::Osmium
{
    extern "C"
    {
        struct ShaderCompilerArgsBinding
        {
            HLSLShaderVersion Version;
            ShaderStage Stage;
            ByteBuffer SourceCode;
            ByteBuffer EntryPoint;
            ByteBuffer FullPath;
        };

        FE_DLL_EXPORT void IShaderCompiler_CompileShader(IShaderCompiler* self, ShaderCompilerArgsBinding* args,
                                                         ByteBuffer* result)
        {
            ShaderCompilerArgs a;
            a.Version    = args->Version;
            a.Stage      = args->Stage;
            a.SourceCode = reinterpret_cast<const char*>(args->SourceCode.Data());
            a.EntryPoint = reinterpret_cast<const char*>(args->EntryPoint.Data());
            a.FullPath   = reinterpret_cast<const char*>(args->FullPath.Data());

            *result = self->CompileShader(a);
        }

        FE_DLL_EXPORT void IShaderCompiler_Destruct(IShaderCompiler* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
